#include <opendaq/dimension_factory.h>

#include <asam_cmp_data_sink/stream_fb.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

StreamFb::StreamFb(const ContextPtr& ctx,
                   const ComponentPtr& parent,
                   const StringPtr& localId,
                   const asam_cmp_common_lib::StreamCommonInit& init,
                   CallsMultiMap& callsMap,
                   const uint16_t& deviceId,
                   const uint32_t& interfaceId)
    : StreamCommonFbImpl(ctx, parent, localId, init)
    , callsMap(callsMap)
    , deviceId(deviceId)
    , interfaceId(interfaceId)
{
    createSignals();
    buildDataDescriptor();
    buildAsyncDomainDescriptor();
}

void StreamFb::setPayloadType(PayloadType type)
{
    updateDescriptors = payloadType == PayloadType::analog || type == PayloadType::analog;
    StreamCommonFbImpl::setPayloadType(type);

    if (payloadType != PayloadType::analog)
    {
        buildDataDescriptor();
        if (updateDescriptors)
        {
            buildAsyncDomainDescriptor();
            updateDescriptors = false;
        }
    }
}

void StreamFb::processData(const std::shared_ptr<Packet>& packet)
{
    if (packet->getPayload().getType() != payloadType)
        return;

    if (payloadType != PayloadType::analog)
        processAsyncData(packet);
    else
        processSyncData(packet);
}

void StreamFb::updateStreamIdInternal()
{
    auto oldStreamId = streamId;
    StreamCommonFbImpl::updateStreamIdInternal();
    if (oldStreamId == streamId)
        return;

    callsMap.erase(deviceId, interfaceId, oldStreamId, this);
    callsMap.insert(deviceId, interfaceId, streamId, this);
}

void StreamFb::createSignals()
{
    dataSignal = createAndAddSignal("data");
    dataSignal.setName("Data");
    domainSignal = createAndAddSignal("time", nullptr, false);
    domainSignal.setName("Time");
    dataSignal.setDomainSignal(domainSignal);
}

void StreamFb::buildDataDescriptor()
{
    switch (payloadType.getType())
    {
        case PayloadType::can:
        case PayloadType::canFd:
            buildCanDescriptor();
            break;
        case PayloadType::analog:
            break;
    }
}

void StreamFb::buildCanDescriptor()
{
    const auto arbIdDescriptor = DataDescriptorBuilder().setName("ArbId").setSampleType(SampleType::Int32).build();
    const auto lengthDescriptor = DataDescriptorBuilder().setName("Length").setSampleType(SampleType::Int8).build();

    const auto dataDescriptor =
        DataDescriptorBuilder()
            .setName("Data")
            .setSampleType(SampleType::UInt8)
            .setDimensions(List<IDimension>(DimensionBuilder().setRule(LinearDimensionRule(0, 1, 64)).setName("Dimension").build()))
            .build();

    const auto canMsgDescriptor = DataDescriptorBuilder()
                                      .setSampleType(SampleType::Struct)
                                      .setStructFields(List<IDataDescriptor>(arbIdDescriptor, lengthDescriptor, dataDescriptor))
                                      .setName("CAN")
                                      .build();

    dataSignal.setDescriptor(canMsgDescriptor);
}
void StreamFb::buildAnalogDescriptor(const AnalogPayload& payload)
{
    auto inputDataType = payload.getSampleDt() == AnalogPayload::SampleDt::aInt16 ? SampleType::Int16 : SampleType::Int32;

    const auto analogDescriptor = DataDescriptorBuilder()
                                      .setName("Analog")
                                      .setSampleType(SampleType::Float64)
                                      .setUnit(AsamCmpToOpenDaqUnit(payload.getUnit()))
                                      .setPostScaling(LinearScaling(payload.getSampleScalar(), payload.getSampleOffset(), inputDataType))
                                      .build();

    dataSignal.setDescriptor(analogDescriptor);
}

void StreamFb::buildAsyncDomainDescriptor()
{
    const auto domainDescriptor = DataDescriptorBuilder()
                                      .setSampleType(SampleType::UInt64)
                                      .setUnit(Unit("s", -1, "seconds", "time"))
                                      .setTickResolution(Ratio(1, 1000000000))
                                      .setOrigin(getEpoch())
                                      .setName("Time")
                                      .build();

    domainSignal.setDescriptor(domainDescriptor);
}

void StreamFb::buildSyncDomainDescriptor(const float sampleInterval)
{
    const auto deltaT = getDeltaT(sampleInterval);

    const auto domainDescriptor = DataDescriptorBuilder()
                                      .setSampleType(SampleType::UInt64)
                                      .setUnit(Unit("s", -1, "seconds", "time"))
                                      .setTickResolution(getResolution())
                                      .setRule(LinearDataRule(deltaT, 0))
                                      .setOrigin(getEpoch())
                                      .setName("Time")
                                      .build();

    domainSignal.setDescriptor(domainDescriptor);
}

void StreamFb::processAsyncData(const std::shared_ptr<Packet>& packet)
{
    constexpr uint64_t newSamples = 1;
    auto timestamp = packet->getTimestamp();

    const auto domainPacket = createAsyncDomainPacket(timestamp, newSamples);
    const auto dataPacket = DataPacketWithDomain(domainPacket, dataSignal.getDescriptor(), newSamples);
    const auto buffer = dataPacket.getRawData();

    switch (payloadType.getType())
    {
        case PayloadType::can:
        case PayloadType::canFd:
            fillCanData(buffer, packet);
            break;
    }

    dataSignal.sendPacket(dataPacket);
    domainSignal.sendPacket(domainPacket);
}

DataPacketPtr StreamFb::createAsyncDomainPacket(uint64_t timestamp, uint64_t sampleCount)
{
    const auto domainPacket = DataPacket(domainSignal.getDescriptor(), sampleCount, timestamp);

    const auto outDomainPacketBuf = static_cast<uint64_t*>(domainPacket.getRawData());
    if (outDomainPacketBuf)
        *outDomainPacketBuf = timestamp;

    return domainPacket;
}

void StreamFb::fillCanData(void* const data, const std::shared_ptr<Packet>& packet)
{
    const auto dataBuffer = static_cast<CANData*>(data);
    auto& payload = static_cast<const CanPayload&>(packet->getPayload());

    dataBuffer->arbId = payload.getId();
    dataBuffer->length = payload.getDataLength();
    memcpy(dataBuffer->data, payload.getData(), dataBuffer->length);
}

void StreamFb::processSyncData(const std::shared_ptr<Packet>& packet)
{
    if (updateDescriptors)
    {
        buildSyncDomainDescriptor(static_cast<const AnalogPayload&>(packet->getPayload()).getSampleInterval());
        buildAnalogDescriptor(static_cast<const AnalogPayload&>(packet->getPayload()));
        updateDescriptors = false;
    }

    const auto& analogPayload = static_cast<const AnalogPayload&>(packet->getPayload());
    auto sampleCount = analogPayload.getSamplesCount();
    auto timestamp = packet->getTimestamp();

    const auto domainPacket = DataPacket(domainSignal.getDescriptor(), sampleCount, timestamp);
    const auto dataPacket = DataPacketWithDomain(domainPacket, dataSignal.getDescriptor(), sampleCount);
    const auto buffer = dataPacket.getRawData();

    memcpy(buffer, analogPayload.getData(), dataPacket.getRawDataSize());

    dataSignal.sendPacket(dataPacket);
    domainSignal.sendPacket(domainPacket);
}

StringPtr StreamFb::getEpoch() const
{
    const std::time_t epochTime = std::chrono::system_clock::to_time_t(std::chrono::time_point<std::chrono::system_clock>{});

    char buf[48];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&epochTime));

    return {buf};
}

RatioPtr StreamFb::getResolution()
{
    return Ratio(1, 1000000000);
}

Int StreamFb::getDeltaT(float sampleInterval)
{
    const double tickPeriod = getResolution();
    const Int deltaT = static_cast<Int>(std::round(sampleInterval / tickPeriod));

    return deltaT;
}

UnitPtr StreamFb::AsamCmpToOpenDaqUnit(AnalogPayload::Unit asamCmpUnit)
{
    switch (asamCmpUnit)
    {
        case AnalogPayload::Unit::kilogram:
            return Unit("kg", -1, "Kilogram", "Mass");
    }
    return UnitPtr();
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
