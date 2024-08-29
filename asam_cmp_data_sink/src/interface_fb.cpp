#include <asam_cmp/interface_payload.h>

#include <asam_cmp_data_sink/interface_fb.h>
#include <asam_cmp_data_sink/stream_fb.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

InterfaceFb::InterfaceFb(const ContextPtr& ctx,
                         const ComponentPtr& parent,
                         const StringPtr& localId,
                         const asam_cmp_common_lib::InterfaceCommonInit& init,
                         const uint16_t& deviceId,
                         CallsMultiMap& callsMap)
    : InterfaceCommonFb(ctx, parent, localId, init)
    , deviceId(deviceId)
    , callsMap(callsMap)
{
}

InterfaceFb::InterfaceFb(const ContextPtr& ctx,
                         const ComponentPtr& parent,
                         const StringPtr& localId,
                         const asam_cmp_common_lib::InterfaceCommonInit& init,
                         const uint16_t& deviceId,
                         CallsMultiMap& callsMap,
                         ASAM::CMP::InterfaceStatus&& ifStatus)
    : InterfaceCommonFb(ctx, parent, localId, init)
    , interfaceStatus(std::move(ifStatus))
    , deviceId(deviceId)
    , callsMap(callsMap)
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
        Int streamId = fb.getPropertyValue("StreamId");
        auto handler = fb.as<IDataHandler>(true);
        callsMap.erase(deviceId, oldInterfaceId, streamId, handler);
        callsMap.insert(deviceId, interfaceId, streamId, handler);
    }
}

void InterfaceFb::addStreamInternal()
{
    auto streamId = streamIdManager->getFirstUnusedId();
    auto newFb = addStreamWithParams<StreamFb>(streamId, callsMap, deviceId, interfaceId);
    callsMap.insert(deviceId, interfaceId, streamId, newFb.as<IDataHandler>(true));
}

void InterfaceFb::removeStreamInternal(size_t nInd)
{
    auto fb = functionBlocks.getItems().getItemAt(nInd);
    Int streamId = fb.getPropertyValue("StreamId");
    InterfaceCommonFb::removeStreamInternal(nInd);
    callsMap.erase(deviceId, interfaceId, streamId, fb.asPtr<IDataHandler>(true));
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
        auto newFb = addStreamWithParams<StreamFb>(newId, callsMap, deviceId, interfaceId);
        callsMap.insert(deviceId, interfaceId, newId, newFb.as<IDataHandler>(true));
    }
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
