#include <asam_cmp_capture_module/stream_fb.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/sample_type_traits.h>
#include <coretypes/enumeration_type_factory.h>
#include <asam_cmp_capture_module/input_descriptors_validator.h>
#include <asam_cmp_capture_module/dispatch.h>
#include <asam_cmp/can_payload.h>
#include <asam_cmp/can_fd_payload.h>
#include <asam_cmp/analog_payload.h>
#include <asam_cmp_common_lib/ethernet_pcpp_itf.h>
#include <asam_cmp_common_lib/unit_converter.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

constexpr std::string_view InputDisconnected{"Disconnected"};
constexpr std::string_view InputConnected{"Connected"};
constexpr std::string_view InputInvalid{"Invalid"};

constexpr std::string_view IsClientScaling{"$IsConnectedAnalogSignal == true && $IsClientPostScaling == false"};
constexpr std::string_view IsClientRange{"$IsConnectedAnalogSignal == true && $IsClientPostScaling == true"};

StreamFb::StreamFb(const ContextPtr& ctx,
                   const ComponentPtr& parent,
                   const StringPtr& localId,
                   const asam_cmp_common_lib::StreamCommonInit& init,
                   const StreamInit& internalInit)
    : asam_cmp_common_lib::StreamCommonFb(ctx, parent, localId, init)
    , streamIdsList(internalInit.streamIdsList)
    , statusSync(internalInit.statusSync)
    , interfaceId(internalInit.interfaceId)
    , ethernetWrapper(internalInit.ethernetWrapper)
    , dataContext(createEncoderDataContext())
    , allowJumboFrames(internalInit.allowJumboFrames)
    , encoders(internalInit.encoderBank)
    , parentInterfaceUpdater(internalInit.parentInterfaceUpdater)
{
    createInputPort();
    initStatuses();
    initProperties();
}

void StreamFb::initProperties()
{
    auto prop = BoolPropertyBuilder("IsConnectedAnalogSignal", false).setReadOnly(true).setVisible(false).build();
    objPtr.addProperty(prop);

    prop = BoolPropertyBuilder("IsClientPostScaling", false).setReadOnly(true).setVisible(false).build();
    objPtr.addProperty(prop);

    auto propName = "MinValue";
    prop = FloatPropertyBuilder(propName, 0).setVisible(EvalValue(IsClientScaling.data())).setReadOnly(true).build();
    objPtr.addProperty(prop);

    propName = "MaxValue";
    prop = FloatPropertyBuilder(propName, 0).setVisible(EvalValue(IsClientScaling.data())).setReadOnly(true).build();
    objPtr.addProperty(prop);

    propName = "Scale";
    prop = FloatPropertyBuilder(propName, 0).setVisible(EvalValue(IsClientRange.data())).setReadOnly(true).build();
    objPtr.addProperty(prop);

    propName = "Offset";
    prop = FloatPropertyBuilder(propName, 0).setVisible(EvalValue(IsClientRange.data())).setReadOnly(true).build();
    objPtr.addProperty(prop);
}

void StreamFb::createInputPort()
{
    inputPort = createAndAddInputPort("input", PacketReadyNotification::Scheduler);
}

void StreamFb::updateStreamIdInternal()
{
    std::scoped_lock lock(statusSync, sync);

    streamIdsList.erase(streamId);
    auto oldId = streamId;
    StreamCommonFbImpl::updateStreamIdInternal();

    if (oldId != streamId)
    {
        streamIdsList.erase(streamId);
        streamIdsList.insert(streamId);
        parentInterfaceUpdater();
    }
}

void StreamFb::initStatuses()
{
    if (!this->context.getTypeManager().hasType("InputStatusType"))
    {
        auto inputStatusType =
            EnumerationType("InputStatusType", List<IString>(InputDisconnected.data(), InputConnected.data(), InputInvalid.data()));
        this->context.getTypeManager().addType(inputStatusType);
    }

    auto thisStatusContainer = this->statusContainer.asPtr<IComponentStatusContainerPrivate>();

    auto inputStatusValue = Enumeration("InputStatusType", InputDisconnected.data(), context.getTypeManager());
    thisStatusContainer.addStatus("InputStatus", inputStatusValue);
}

void StreamFb::setInputStatus(const StringPtr& value)
{
    auto thisStatusContainer = this->statusContainer.asPtr<IComponentStatusContainerPrivate>();

    auto inputStatusValue = Enumeration("InputStatusType", value, context.getTypeManager());
    thisStatusContainer.setStatus("InputStatus", inputStatusValue);
}

void StreamFb::configureScaledAnalogSignal()
{
    auto postScaling = inputDataDescriptor.getPostScaling();
    auto params = postScaling.getParameters();
    analogDataScale = params.get("scale");
    analogDataOffset = params.get("offset");

    analogDataSampleDt = postScaling.getInputSampleType() == SampleType::Int16 ? 16 : 32;
    analogDataHasInternalPostScaling = false;

    setPropertyValueInternal(
        String("Scale").asPtr<IString>(true), BaseObjectPtr(analogDataScale).asPtr<IBaseObject>(true), false, true, false);

    setPropertyValueInternal(
        String("Offset").asPtr<IString>(true), BaseObjectPtr(analogDataOffset).asPtr<IBaseObject>(true), false, true, false);

    setPropertyValueInternal(
        String("IsClientPostScaling").asPtr<IString>(true), BaseObjectPtr(true).asPtr<IBaseObject>(true), false, true, false);
}

void StreamFb::configureMinMaxAnalogSignal()
{
    auto sampleType = inputDataDescriptor.getSampleType();
    auto range = inputDataDescriptor.getValueRange();


    if (hasCorrectValueRange(range))
    {
        analogDataMin = range.getLowValue();
        analogDataMax = range.getHighValue();
    }
    else
    {
        if (sampleType == SampleType::Int16)
        {
            analogDataMin = std::numeric_limits<int16_t>::min();
            analogDataMax = std::numeric_limits<int16_t>::max();
        }
        else
        {
            analogDataMin = std::numeric_limits<int32_t>::min();
            analogDataMax = std::numeric_limits<int32_t>::max();
        }
    }

    if (hasCorrectSampleType(sampleType))
    {
        analogDataScale = 1;
        analogDataOffset = 0;
        analogDataSampleDt = (sampleType == SampleType::Int16 ? 16 : 32);
        analogDataHasInternalPostScaling = false;
    }
    else
    {
        analogDataSampleDt = 32;
        analogDataScale = (analogDataMax - analogDataMin) / (1LL << 24);
        analogDataOffset = analogDataMin;
        analogDataHasInternalPostScaling = true;
    }

    setPropertyValueInternal(
        String("MinValue").asPtr<IString>(true), BaseObjectPtr(analogDataMin).asPtr<IBaseObject>(true), false, true, false);

    setPropertyValueInternal(
        String("MaxValue").asPtr<IString>(true), BaseObjectPtr(analogDataMax).asPtr<IBaseObject>(true), false, true, false);

    setPropertyValueInternal(
        String("IsClientPostScaling").asPtr<IString>(true), BaseObjectPtr(false).asPtr<IBaseObject>(true), false, true, false);
}

void StreamFb::onAnalogSignalConnected()
{
    setPropertyValueInternal(
        String("IsConnectedAnalogSignal").asPtr<IString>(true), BaseObjectPtr(true).asPtr<IBaseObject>(true), false, true, false);

    constexpr double invertedTickResolution = 1'000'000;
    int64_t delta = inputDomainDataDescriptor.getRule().getParameters().get("delta");
    analogDataDeltaTime = delta / invertedTickResolution;

    if (hasCorrectPostScaling(inputDataDescriptor.getPostScaling()) )
    {
        configureScaledAnalogSignal();
    }
    else
    {
        configureMinMaxAnalogSignal();
    }
}

void StreamFb::onAnalogSignalDisconnected()
{
    setPropertyValueInternal(
        String("IsConnectedAnalogSignal").asPtr<IString>(true), BaseObjectPtr(false).asPtr<IBaseObject>(true), false, true, false);
}

void StreamFb::onDisconnected(const InputPortPtr& inputPort)
{
    if (this->inputPort == inputPort)
    {
        if (payloadType == ASAM::CMP::PayloadType::analog)
            onAnalogSignalDisconnected();
        setInputStatus(InputDisconnected.data());
    }
}

void StreamFb::onPacketReceived(const InputPortPtr& port)
{
    std::scoped_lock lock{sync};

    PacketPtr packet;
    const auto connection = inputPort.getConnection();
    if (!connection.assigned())
        return;

    packet = connection.dequeue();

    while (packet.assigned())
    {
        switch (packet.getType())
        {
            case PacketType::Event:
                processEventPacket(packet);
                break;

            case PacketType::Data:
                processDataPacket(packet);
                break;

            default:
                break;
        }

        packet = connection.dequeue();
    };
}

void StreamFb::processSignalDescriptorChanged(DataDescriptorPtr inputDataDescriptor, DataDescriptorPtr inputDomainDataDescriptor)
{
    if (inputDataDescriptor.assigned())
        this->inputDataDescriptor = inputDataDescriptor;
    if (inputDomainDataDescriptor.assigned())
        this->inputDomainDataDescriptor = inputDomainDataDescriptor;

    configure();
}

void StreamFb::configure()
{
    if (!inputDataDescriptor.assigned() || !inputDomainDataDescriptor.assigned())
    {
        setInputStatus(InputInvalid.data());
        LOG_D("Incomplete signal descriptors")
        return;
    }

    try
    {
        if (!validateInputDescriptor(inputDataDescriptor, payloadType))
            throw std::runtime_error("Invalid data descriptor fields structure");

        if (payloadType == ASAM::CMP::PayloadType::analog)
            onAnalogSignalConnected();

        setInputStatus(InputConnected.data());
    }
    catch (const std::exception& e)
    {
        LOG_W("Failed to set descriptor for trigger signal: {}", e.what())
        setInputStatus(InputInvalid.data());
    }
}

void StreamFb::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        processSignalDescriptorChanged(inputDataDescriptor, inputDomainDataDescriptor);
    }
}


ASAM::CMP::DataContext StreamFb::createEncoderDataContext() const
{
    constexpr int minFrameSize{64}, maxFrameSize{1500};
    assert(!allowJumboFrames);
    return {minFrameSize, maxFrameSize};
}

void StreamFb::processCanPacket(const DataPacketPtr& packet)
{
#pragma pack(push, 1)
    struct CANData
    {
        uint32_t arbId;
        uint8_t length;
        uint8_t data[64];
    };
#pragma pack(pop)

    auto* canData = reinterpret_cast<CANData*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();

    uint64_t* rawTimeBuffer = reinterpret_cast<uint64_t*>(packet.getDomainPacket().getRawData());
    RatioPtr timeResolution = packet.getDomainPacket().getDataDescriptor().getTickResolution();
    size_t timeScale = 1'000'000'000 / timeResolution.getDenominator();

    std::vector<ASAM::CMP::Packet> packets;
    packets.reserve(sampleCount);

    for (size_t i = 0; i < sampleCount; i++)
    {
        if (canData->length <= 8)
        {
            ASAM::CMP::CanPayload payload;
            payload.setData(canData->data, canData->length);
            payload.setId(canData->arbId);

            packets.emplace_back();
            packets.back().setInterfaceId(interfaceId);
            packets.back().setPayload(payload);
            packets.back().setTimestamp((*rawTimeBuffer) * timeScale);
        }
        canData++;
        rawTimeBuffer++;
    }

    for (auto& rawFrame : encoders->encode(streamId, packets.begin(), packets.end(), dataContext))
        ethernetWrapper->sendPacket(rawFrame);
}

void StreamFb::processCanFdPacket(const DataPacketPtr& packet)
{
#pragma pack(push, 1)
    struct CANData
    {
        uint32_t arbId;
        uint8_t length;
        uint8_t data[64];
    };
#pragma pack(pop)

    auto* canData = reinterpret_cast<CANData*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();

    uint64_t* rawTimeBuffer = reinterpret_cast<uint64_t*>(packet.getDomainPacket().getRawData());
    RatioPtr timeResolution = packet.getDomainPacket().getDataDescriptor().getTickResolution();
    size_t timeScale = 1'000'000'000 / timeResolution.getDenominator();

    std::vector<ASAM::CMP::Packet> packets;
    packets.reserve(sampleCount);

    for (size_t i = 0; i < sampleCount; i++)
    {
        ASAM::CMP::CanFdPayload payload;
        payload.setData(canData->data, canData->length);
        payload.setId(canData->arbId);

        packets.emplace_back();
        packets.back().setInterfaceId(interfaceId);
        packets.back().setPayload(payload);
        packets.back().setTimestamp((*rawTimeBuffer) * timeScale);

        canData++;
        rawTimeBuffer++;
    }

    for (auto& rawFrame : encoders->encode(streamId, packets.begin(), packets.end(), dataContext))
        ethernetWrapper->sendPacket(rawFrame);
}

template <SampleType SrcType>
void createAnalogPayloadWithInternalScaling(ASAM::CMP::AnalogPayload& payload,
                                            const daq::DataPacketPtr& packet,
                                            Float analogDataScale,
                                            Float analogDataOffset,
                                            Float analogDataDeltaTime)
{
    payload.setSampleInterval(analogDataDeltaTime);

    using SourceType = typename SampleTypeToType<SrcType>::Type;
    auto* rawData = reinterpret_cast<SourceType*>(packet.getRawData());
    const size_t sampleCount = packet.getSampleCount();
    const size_t sampleSize = getSampleSize(SrcType);

    uint8_t unitId = asam_cmp_common_lib::Units::getIdBySymbol(packet.getDataDescriptor().getUnit().getSymbol().toStdString());
    payload.setUnit(ASAM::CMP::AnalogPayload::Unit(unitId));
    payload.setSampleDt(ASAM::CMP::AnalogPayload::SampleDt::aInt32);
    payload.setSampleScalar(analogDataScale);
    payload.setSampleOffset(analogDataOffset);

    std::vector<int32_t> scaledData(sampleCount);
    for (int i = 0; i < sampleCount; ++i)
    {
        scaledData[i] = std::round((rawData[i] - analogDataOffset) / analogDataScale);
    }

    payload.setData(reinterpret_cast<uint8_t*>(scaledData.data()), sampleCount * sizeof(int32_t));
}

void createAnalogPayload(ASAM::CMP::AnalogPayload& payload,
                                             const DataPacketPtr& packet,
                                             const DataDescriptorPtr& inputDataDescriptor,
                                             Float analogDataScale,
                                             Float analogDataOffset,
                                             Float analogDataDeltaTime,
                                             Int analogDataSampleDt)
{
    payload.setSampleInterval(analogDataDeltaTime);

    auto* rawData = reinterpret_cast<uint8_t*>(packet.getRawData());
    const size_t sampleCount = packet.getSampleCount();
    const auto inputSampleType = inputDataDescriptor.getPostScaling().assigned() ? inputDataDescriptor.getPostScaling().getInputSampleType()
                                                                                 : inputDataDescriptor.getSampleType();
    const size_t sampleSize = inputSampleType == SampleType::Int16 ? 2 : 4;

    uint8_t unitId = asam_cmp_common_lib::Units::getIdBySymbol(packet.getDataDescriptor().getUnit().getSymbol().toStdString());
    payload.setUnit(ASAM::CMP::AnalogPayload::Unit(unitId));
    payload.setSampleDt(analogDataSampleDt == 16 ? ASAM::CMP::AnalogPayload::SampleDt::aInt16 : ASAM::CMP::AnalogPayload::SampleDt::aInt32);
    payload.setSampleScalar(analogDataScale);
    payload.setSampleOffset(analogDataOffset);

    payload.setData(rawData, sampleCount * sampleSize);

}

void StreamFb::processAnalogPacket(const DataPacketPtr& packet)
{
    ASAM::CMP::AnalogPayload payload;
    if (analogDataHasInternalPostScaling)
        SAMPLE_TYPE_DISPATCH(
            inputDataDescriptor.getSampleType(), createAnalogPayloadWithInternalScaling, payload, packet, analogDataScale, analogDataOffset, analogDataDeltaTime)
    else
        createAnalogPayload(payload, packet, inputDataDescriptor, analogDataScale, analogDataOffset, analogDataDeltaTime, analogDataSampleDt);

    ASAM::CMP::Packet asamCmpPacket;
    asamCmpPacket.setInterfaceId(interfaceId);
    asamCmpPacket.setPayload(payload);

    auto domainPacket = packet.getDomainPacket();
    uint64_t rawTime = domainPacket.getOffset();
    RatioPtr timeResolution = packet.getDomainPacket().getDataDescriptor().getTickResolution();
    size_t timeScale = 1'000'000'000 / timeResolution.getDenominator();
    asamCmpPacket.setTimestamp(rawTime * timeScale);

    for (auto& rawFrame : encoders->encode(streamId, asamCmpPacket, dataContext))
        ethernetWrapper->sendPacket(rawFrame);
}

void StreamFb::processDataPacket(const DataPacketPtr& packet)
{
    switch (payloadType.getType())
    {
        case ASAM::CMP::PayloadType::can:
            processCanPacket(packet);
            break;
        case ASAM::CMP::PayloadType::canFd:
            processCanFdPacket(packet);
            break;
        case ASAM::CMP::PayloadType::analog:
            processAnalogPacket(packet);
            break;
    }
}

void StreamFb::setPayloadType(ASAM::CMP::PayloadType type)
{
    std::scoped_lock lock(sync);
    asam_cmp_common_lib::StreamCommonFb::setPayloadType(type);

    if (payloadType != type)
    {
        setInputStatus(InputDisconnected.data());
    }
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
