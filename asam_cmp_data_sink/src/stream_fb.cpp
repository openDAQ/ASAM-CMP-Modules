#include <asam_cmp/can_payload.h>
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
    buildDomainDescriptor();
}

void StreamFb::setPayloadType(PayloadType type)
{
    StreamCommonFbImpl::setPayloadType(type);
    buildDataDescriptor();
}

void StreamFb::processData(const std::shared_ptr<ASAM::CMP::Packet>& packet)
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
    domainSignal = createAndAddSignal("domain", nullptr, false);
    domainSignal.setName("Domain");
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
    }
}

void StreamFb::buildDomainDescriptor()
{
    const auto domainDescriptor = DataDescriptorBuilder()
                                      .setSampleType(SampleType::UInt64)
                                      .setUnit(Unit("s", -1, "seconds", "time"))
                                      .setTickResolution(Ratio(1, 1000000000))
                                      .setOrigin(getEpoch())
                                      .setName("Time");

    domainSignal.setDescriptor(domainDescriptor.build());
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

void StreamFb::processAsyncData(const std::shared_ptr<ASAM::CMP::Packet>& packet)
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

void StreamFb::fillCanData(void* const data, const std::shared_ptr<ASAM::CMP::Packet>& packet)
{
    const auto dataBuffer = static_cast<CANData*>(data);
    auto& payload = static_cast<const ASAM::CMP::CanPayload&>(packet->getPayload());

    dataBuffer->arbId = payload.getId();
    dataBuffer->length = payload.getDataLength();
    memcpy(dataBuffer->data, payload.getData(), dataBuffer->length);
}

void StreamFb::processSyncData(const std::shared_ptr<ASAM::CMP::Packet>& packet)
{
}

StringPtr StreamFb::getEpoch() const
{
    const std::time_t epochTime = std::chrono::system_clock::to_time_t(std::chrono::time_point<std::chrono::system_clock>{});

    char buf[48];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&epochTime));

    return {buf};
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
