#include <asam_cmp_capture_module/stream_fb.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

StreamFb::StreamFb(const ContextPtr& ctx,
                   const ComponentPtr& parent,
                   const StringPtr& localId,
                   const asam_cmp_common_lib::StreamCommonInit& init)
    : asam_cmp_common_lib::StreamCommonFb(ctx, parent, localId, init)
{
    createInputPort();
}

void StreamFb::createInputPort()
{
    inputPort = createAndAddInputPort("input", PacketReadyNotification::Scheduler);
}


END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
