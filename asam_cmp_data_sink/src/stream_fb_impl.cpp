#include <asam_cmp_data_sink/stream_fb_impl.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

StreamFbImpl::StreamFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const AsamCmpStreamCommonInit& init)
    : StreamCommonFbImpl(ctx, parent, localId, init)
{
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
