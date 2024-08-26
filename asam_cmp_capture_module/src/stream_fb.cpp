#include <asam_cmp_capture_module/stream_fb.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <coretypes/enumeration_type_factory.h>
#include <asam_cmp_capture_module/input_descriptors_validator.h>
#include <asam_cmp/can_payload.h>
#include <asam_cmp/can_fd_payload.h>
#include <asam_cmp/analog_payload.h>
#include <asam_cmp_common_lib/ethernet_pcpp_itf.h>
#include <asam_cmp_common_lib/unit_converter.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

constexpr std::string_view InputDisconnected{"Disconnected"};
constexpr std::string_view InputConnected{"Connected"};
constexpr std::string_view InputInvalid{"Invalid"};

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

void StreamFb::onDisconnected(const InputPortPtr& inputPort)
{
    if (this->inputPort == inputPort)
    {
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

        configureCustomParameters();

        setInputStatus(InputConnected.data());
    }
    catch (const std::exception& e)
    {
        LOG_W("Failed to set descriptor for trigger signal: {}", e.what())
        setInputStatus(InputInvalid.data());
    }
}

void StreamFb::configureCustomParameters()
{
    switch (payloadType.getType())
    {
        case ASAM::CMP::PayloadType::analog:
            constexpr double invertedTickResolution = 1'000'000;
            int64_t delta = inputDomainDataDescriptor.getRule().getParameters().get("delta");
            deltaTime = delta/invertedTickResolution;
            break;
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

void StreamFb::processAnalogPacket(const DataPacketPtr& packet, bool isCanFd)
{
    auto* rawData = reinterpret_cast<uint8_t*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();
    const size_t sampleSize = inputDataDescriptor.getSampleSize();

    uint64_t* rawTimeBuffer = reinterpret_cast<uint64_t*>(packet.getDomainPacket().getRawData());

    ASAM::CMP::AnalogPayload payload;

    payload.setSampleInterval(deltaTime);
    payload.setData(rawData, sampleCount * sampleSize);
    uint8_t unitId = asam_cmp_common_lib::Units::getIdBySymbol(packet.getDataDescriptor().getUnit().getSymbol().toStdString());
    payload.setUnit(ASAM::CMP::AnalogPayload::Unit(unitId));
    float sampleScalar = packet.getDataDescriptor().getPostScaling().getParameters().get("scale");
    float sampleOffset = packet.getDataDescriptor().getPostScaling().getParameters().get("offset");
    payload.setSampleScalar(sampleScalar);
    payload.setSampleOffset(sampleOffset);

    ASAM::CMP::Packet asamCmpPacket;
    asamCmpPacket.setInterfaceId(interfaceId);
    asamCmpPacket.setPayload(payload);

    RatioPtr timeResolution = packet.getDomainPacket().getDataDescriptor().getTickResolution();
    size_t timeScale = 1'000'000'000 / timeResolution.getDenominator();
    asamCmpPacket.setTimestamp((*rawTimeBuffer) * timeScale);
    // TODO: Add SampleDT - in standard we have A_INT16 and A_INT32 probably validator should be updated

    for (auto& rawFrame : encoders->encode(streamId, asamCmpPacket, dataContext))
        ethernetWrapper->sendPacket(selectedDeviceName, rawFrame);
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
            processAnalogPacket(packet, true);
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
