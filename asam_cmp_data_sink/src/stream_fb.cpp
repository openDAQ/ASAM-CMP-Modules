#include <opendaq/dimension_factory.h>

#include <asam_cmp_common_lib/unit_converter.h>
#include <asam_cmp_data_sink/stream_fb.h>
#include <opendaq/binary_data_packet_factory.h>


#include <chrono>


BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

StreamFb::StreamFb(const ContextPtr& ctx,
                   const ComponentPtr& parent,
                   const StringPtr& localId,
                   const asam_cmp_common_lib::StreamCommonInit& init,
                   DataPacketsPublisher& publisher,
                   const uint16_t& deviceId,
                   const uint32_t& interfaceId)
    : StreamCommonFbImpl(ctx, parent, localId, init)
    , deviceId(deviceId)
    , interfaceId(interfaceId)
    , publisher(publisher)
    , updateDescriptors(init.payloadType == PayloadType::analog)
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

void StreamFb::receive(const std::shared_ptr<Packet>& packet)
{
    if (packet->getPayload().getType() != payloadType)
        return;

    if (payloadType == PayloadType::analog)
        processSyncData(packet);
    else
    {
        std::vector<std::shared_ptr<Packet>> packets;
        packets.emplace_back(packet);
        receive(packets);
    }
}

void StreamFb::receive(const std::vector<std::shared_ptr<Packet>>& packets)
{
    if (packets.front()->getPayload().getType() != payloadType)
        return;

    switch (payloadType.getType())
    {
        case PayloadType::analog:
            for (auto& packet : packets)
                processSyncData(packet);
            break;
        case PayloadType::can:
        case PayloadType::canFd:
            processCanData(packets);
            break;
        case PayloadType::ethernet:
            processEthernetData(packets);
            break;
    }
}

void StreamFb::updateStreamIdInternal()
{
    const auto oldStreamId = streamId;
    StreamCommonFbImpl::updateStreamIdInternal();
    if (oldStreamId == streamId)
        return;

    publisher.unsubscribe({deviceId, interfaceId, oldStreamId}, this);
    publisher.subscribe({deviceId, interfaceId, streamId}, this);
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
        case PayloadType::ethernet:
            buildEthernetDescriptor();
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

void StreamFb::buildEthernetDescriptor()
{
    auto metadata = Dict<IString, IString>();
    metadata["DataType"] = "Ethernet";
    const auto ethernetMsgDescriptor =
        DataDescriptorBuilder()
        .setSampleType(SampleType::Binary)
        .setMetadata(metadata)
        .build();

    dataSignal.setDescriptor(ethernetMsgDescriptor);
}

void StreamFb::buildAnalogDescriptor(const AnalogPayload& payload)
{
    const auto inputDataType = payload.getSampleDt() == AnalogPayload::SampleDt::aInt16 ? SampleType::Int16 : SampleType::Int32;
    const auto scalar = payload.getSampleScalar();
    const auto offset = payload.getSampleOffset();
    const auto minValue = offset;
    const auto intSize = inputDataType == SampleType::Int16 ? 16 : 32;
    const auto maxValue = scalar * pow(2, intSize) + offset;

    const auto analogDescriptor = DataDescriptorBuilder()
                                      .setName("Analog")
                                      .setSampleType(SampleType::Float64)
                                      .setUnit(asamCmpToOpenDaqUnit(payload.getUnit()))
                                      .setPostScaling(LinearScaling(scalar, offset, inputDataType))
                                      .setValueRange(Range(minValue, maxValue))
                                      .build();
    dataSignal.setDescriptor(analogDescriptor);

    analogHeader.setSampleDt(payload.getSampleDt());
    analogHeader.setUnit(payload.getUnit());
    analogHeader.setSampleOffset(payload.getSampleOffset());
    analogHeader.setSampleScalar(payload.getSampleScalar());
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

    analogHeader.setSampleInterval(sampleInterval);
}

void StreamFb::processCanData(const std::vector<std::shared_ptr<Packet>>& packets)
{
    const uint64_t newSamples = packets.size();
    auto timestamp = packets.front()->getTimestamp();

    const auto domainPacket = DataPacket(domainSignal.getDescriptor(), newSamples, timestamp);
    auto domainBuffer = static_cast<uint64_t*>(domainPacket.getRawData());

    const auto dataPacket = DataPacketWithDomain(domainPacket, dataSignal.getDescriptor(), newSamples);
    auto buffer = reinterpret_cast<CANData*>(dataPacket.getRawData());

    for (auto& packet : packets)
    {
        auto& payload = static_cast<const CanPayload&>(packet->getPayload());
        buffer->arbId = payload.getId();
        buffer->length = payload.getDataLength();
        memcpy(buffer->data, payload.getData(), buffer->length);

        *domainBuffer++ = packet->getTimestamp();
        buffer++;
    }

    dataSignal.sendPacket(dataPacket);
    domainSignal.sendPacket(domainPacket);
}

void StreamFb::processEthernetData(const std::vector<std::shared_ptr<Packet>>& packets)
{
    static constexpr size_t analogMessagePayloadSize{6};
    for (auto& packet : packets)
    {
        auto timestamp = packet->getTimestamp();

        const auto domainPacket = DataPacket(domainSignal.getDescriptor(), 1, timestamp);
        auto domainBuffer = static_cast<uint64_t*>(domainPacket.getRawData());

        auto& payload = static_cast<const EthernetPayload&>(packet->getPayload());
        auto payloadLen = payload.getLength() - analogMessagePayloadSize;

        const auto dataPacket = BinaryDataPacket(domainPacket, dataSignal.getDescriptor(), payloadLen);
        auto buffer = static_cast<uint8_t*>(dataPacket.getRawData());
        memcpy(buffer, payload.getData(), payloadLen);
        *domainBuffer = packet->getTimestamp();

        dataSignal.sendPacket(dataPacket);
        domainSignal.sendPacket(domainPacket);
    }
}

void StreamFb::processSyncData(const std::shared_ptr<Packet>& packet)
{
    auto& analogPayload = static_cast<const AnalogPayload&>(packet->getPayload());

    if (updateDescriptors)
    {
        buildSyncDomainDescriptor(analogPayload.getSampleInterval());
        buildAnalogDescriptor(analogPayload);
        updateDescriptors = false;
    }
    else
    {
        if (domainChanged(analogPayload))
        {
            buildSyncDomainDescriptor(analogPayload.getSampleInterval());
        }
        if (dataChanged(analogPayload))
        {
            buildAnalogDescriptor(analogPayload);
        }
    }

    const auto sampleCount = analogPayload.getSamplesCount();
    const auto timestamp = packet->getTimestamp();

    const auto domainPacket = DataPacket(domainSignal.getDescriptor(), sampleCount, timestamp);
    const auto dataPacket = DataPacketWithDomain(domainPacket, dataSignal.getDescriptor(), sampleCount);
    const auto buffer = dataPacket.getRawData();

    memcpy(buffer, analogPayload.getData(), dataPacket.getRawDataSize());

    dataSignal.sendPacket(dataPacket);
    domainSignal.sendPacket(domainPacket);
}

bool StreamFb::domainChanged(const AnalogPayload& payload)
{
    return payload.getSampleInterval() != analogHeader.getSampleInterval();
}

bool StreamFb::dataChanged(const AnalogPayload& payload)
{
    return payload.getSampleDt() != analogHeader.getSampleDt() || payload.getUnit() != analogHeader.getUnit() ||
           payload.getSampleOffset() != analogHeader.getSampleOffset() || payload.getSampleScalar() != analogHeader.getSampleScalar();
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

UnitPtr StreamFb::asamCmpToOpenDaqUnit(AnalogPayload::Unit asamCmpUnit)
{
    const auto symbol = asam_cmp_common_lib::Units::getSymbolById(to_underlying(asamCmpUnit));
    return Unit(symbol, -1, "", "");
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
