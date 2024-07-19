#include <asam_cmp_data_sink/interface_fb_impl.h>
#include <asam_cmp_data_sink/stream_fb_impl.h>

#include <asam_cmp/interface_payload.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

InterfaceFbImpl::InterfaceFbImpl(const ContextPtr& ctx,
                                 const ComponentPtr& parent,
                                 const StringPtr& localId,
                                 const AsamCmpInterfaceCommonInit& init)
    : InterfaceCommonFbImpl(ctx, parent, localId, init)
{
}

InterfaceFbImpl::InterfaceFbImpl(const ContextPtr& ctx,
                                 const ComponentPtr& parent,
                                 const StringPtr& localId,
                                 const AsamCmpInterfaceCommonInit& init,
                                 ASAM::CMP::InterfaceStatus&& ifStatus)
    : InterfaceCommonFbImpl(ctx, parent, localId, init)
    , interfaceStatus(std::move(ifStatus))
{
    createFbs();
}

void InterfaceFbImpl::addStreamInternal()
{
    auto id = streamIdManager->getFirstUnusedId();
    addStreamWithParams<StreamFbImpl>(id);
}

void InterfaceFbImpl::createFbs()
{
    const auto& ifPayload = static_cast<ASAM::CMP::InterfacePayload&>(interfaceStatus.getPacket().getPayload());
    auto streamIds = ifPayload.getStreamIds();
    for (uint16_t i = 0; i < ifPayload.getStreamIdsCount(); ++i)
    {
        auto newId = streamIds[i];
        addStreamWithParams<StreamFbImpl>(newId);
    }
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
