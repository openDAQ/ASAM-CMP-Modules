#include <asam_cmp_capture_module/capture_module_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

CaptureModuleImpl::CaptureModuleImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();
}

FunctionBlockTypePtr CaptureModuleImpl::CreateType()
{
    return FunctionBlockType("capture_module", "CaptureModule", "Capture Module");
}

void CaptureModuleImpl::initProperties()
{
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
