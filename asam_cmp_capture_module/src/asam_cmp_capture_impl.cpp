#include <asam_cmp_capture_module/asam_cmp_capture_impl.h>
#include <string_view>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

AsamCmpCaptureImpl::AsamCmpCaptureImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();
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
    // const StringPtr dataSinkId = "asam_cmp_data_sink";
    // auto newFb = createWithImplementation<IFunctionBlock, CaptureModuleFbImpl>(context, functionBlocks, dataSinkId);
    // functionBlocks.addItem(newFb);
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
