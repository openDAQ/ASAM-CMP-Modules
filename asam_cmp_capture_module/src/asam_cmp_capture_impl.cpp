#include <asam_cmp_capture_module/asam_cmp_capture_impl.h>
#include <asam_cmp_capture_module/capture_module_impl.h>
#include <string_view>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

AsamCmpCaptureImpl::AsamCmpCaptureImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();
    createFbs();
}

FunctionBlockTypePtr AsamCmpCaptureImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_capture", "AsamCmpCapture", "Asam CMP Capture");
}

void AsamCmpCaptureImpl::initProperties()
{

}

void AsamCmpCaptureImpl::createFbs()
{
    const StringPtr captureModuleId = "capture_module";
    auto newFb = createWithImplementation<IFunctionBlock, CaptureModuleImpl>(context, functionBlocks, captureModuleId);
    functionBlocks.addItem(newFb);
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
