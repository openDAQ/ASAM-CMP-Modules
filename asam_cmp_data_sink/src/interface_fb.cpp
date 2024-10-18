#include <asam_cmp/interface_payload.h>

#include <asam_cmp_data_sink/interface_fb.h>
#include <asam_cmp_data_sink/stream_fb.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

InterfaceFb::InterfaceFb(const ContextPtr& ctx,
                         const ComponentPtr& parent,
                         const StringPtr& localId,
                         const asam_cmp_common_lib::InterfaceCommonInit& init,
                         const uint16_t& deviceId,
                         DataPacketsPublisher& publisher)
    : InterfaceCommonFb(ctx, parent, localId, init)
    , deviceId(deviceId)
    , publisher(publisher)
{
}

InterfaceFb::InterfaceFb(const ContextPtr& ctx,
                         const ComponentPtr& parent,
                         const StringPtr& localId,
                         const asam_cmp_common_lib::InterfaceCommonInit& init,
                         const uint16_t& deviceId,
                         DataPacketsPublisher& publisher,
                         ASAM::CMP::InterfaceStatus&& ifStatus)
    : InterfaceCommonFb(ctx, parent, localId, init)
    , interfaceStatus(std::move(ifStatus))
    , deviceId(deviceId)
    , publisher(publisher)
{
    createFbs();
}

void InterfaceFb::updateInterfaceIdInternal()
{
    auto oldInterfaceId = interfaceId;
    InterfaceCommonFb::updateInterfaceIdInternal();
    if (oldInterfaceId == interfaceId)
        return;

    for (const auto& fb : functionBlocks.getItems())
    {
        uint8_t streamId = static_cast<Int>(fb.getPropertyValue("StreamId"));
        auto handler = fb.as<IAsamCmpPacketsSubscriber>(true);
        publisher.unsubscribe({deviceId, oldInterfaceId, streamId}, handler);
        publisher.subscribe({deviceId, interfaceId, streamId}, handler);
    }
}

void InterfaceFb::addStreamInternal()
{
    auto streamId = streamIdManager.getFirstUnusedId();
    auto newFb = addStreamWithParams<StreamFb>(streamId, publisher, deviceId, interfaceId);
    publisher.subscribe({deviceId, interfaceId, streamId}, newFb.as<IAsamCmpPacketsSubscriber>(true));
}

void InterfaceFb::removeStreamInternal(size_t nInd)
{
    auto fb = functionBlocks.getItems().getItemAt(nInd);
    uint8_t streamId = static_cast<Int>(fb.getPropertyValue("StreamId"));
    InterfaceCommonFb::removeStreamInternal(nInd);
    publisher.unsubscribe({deviceId, interfaceId, streamId}, fb.asPtr<IAsamCmpPacketsSubscriber>(true));
}

void InterfaceFb::createFbs()
{
    const auto& ifPayload = static_cast<ASAM::CMP::InterfacePayload&>(interfaceStatus.getPacket().getPayload());
    payloadType.setMessageType(ASAM::CMP::CmpHeader::MessageType::data);
    payloadType.setRawPayloadType(ifPayload.getInterfaceType());
    if (payloadType.isValid())
        objPtr.setPropertyValue("PayloadType", asamPayloadTypeToPayloadType.at(payloadType.getType()));
    auto streamIds = ifPayload.getStreamIds();
    for (uint16_t i = 0; i < ifPayload.getStreamIdsCount(); ++i)
    {
        auto newId = streamIds[i];
        auto newFb = addStreamWithParams<StreamFb>(newId, publisher, deviceId, interfaceId);
        publisher.subscribe({deviceId, interfaceId, newId}, newFb.as<IAsamCmpPacketsSubscriber>(true));
    }
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
