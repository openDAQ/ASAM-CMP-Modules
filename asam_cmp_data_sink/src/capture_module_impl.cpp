#include <asam_cmp_data_sink/capture_module_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

CaptureModuleImpl::CaptureModuleImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();
}

CaptureModuleImpl::CaptureModuleImpl(const ContextPtr& ctx,
                                     const ComponentPtr& parent,
                                     const StringPtr& localId,
                                     ASAM::CMP::DeviceStatus&& deviceStatus)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , deviceStatus(std::move(deviceStatus))
{
    initProperties();
    createFbs();
}

FunctionBlockTypePtr CaptureModuleImpl::CreateType()
{
    return FunctionBlockType("capture_module", "CaptureModule", "Capture Module");
}

void CaptureModuleImpl::initProperties()
{
}

void CaptureModuleImpl::createFbs()
{
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
