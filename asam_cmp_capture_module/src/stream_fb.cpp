#include <asam_cmp_capture_module/stream_fb.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

StreamFb::StreamFb(const ContextPtr& ctx,
                   const ComponentPtr& parent,
                   const StringPtr& localId,
                   const asam_cmp_common_lib::StreamCommonInit& init,
                   const StreamInit& internalInit)
    : asam_cmp_common_lib::StreamCommonFb(ctx, parent, localId, init)
    , streamIdsList(internalInit.streamIdsList)
    , statusSync(internalInit.statusSync)
{
    createInputPort();
}

void StreamFb::createInputPort()
{
    inputPort = createAndAddInputPort("input", PacketReadyNotification::Scheduler);
}

void StreamFb::updateStreamIdInternal()
{
    if (isInternalUpdate)
        return;

    std::scoped_lock lock(statusSync);

    streamIdsList.erase(streamId);
    streamIdsList.insert(objPtr.getPropertyValue("StreamId"));
    StreamCommonFbImpl::updateStreamIdInternal();
}


END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
