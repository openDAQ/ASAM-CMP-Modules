#include <asam_cmp_data_sink/stream_fb.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

#include <asam_cmp_common_lib/id_manager.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

StreamFb::StreamFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const asam_cmp_common_lib::StreamCommonInit& init)
    : StreamCommonFb(ctx, parent, localId, init)
{
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
