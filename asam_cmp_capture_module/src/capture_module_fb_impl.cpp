#include <asam_cmp_capture_module/capture_module_fb_impl.h>
#include <asam_cmp_capture_module/capture_fb_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

CaptureModuleFbImpl::CaptureModuleFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();
    createFbs();
}

FunctionBlockTypePtr CaptureModuleFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_capture_module_fb", "AsamCmpCaptureModuleFb", "Asam CMP Capture Module Fb");
}

void CaptureModuleFbImpl::initProperties()
{

}

void CaptureModuleFbImpl::createFbs()
{
    const StringPtr captureModuleId = "asam_cmp_capture_fb";
    auto newFb = createWithImplementation<IFunctionBlock, CaptureFbImpl>(context, functionBlocks, captureModuleId);
    functionBlocks.addItem(newFb);
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
