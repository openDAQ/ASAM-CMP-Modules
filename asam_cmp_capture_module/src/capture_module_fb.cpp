#include <asam_cmp_capture_module/capture_fb.h>
#include <asam_cmp_capture_module/capture_module_fb.h>
#include <asam_cmp_common_lib/ethernet_pcpp_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

CaptureModuleFb::CaptureModuleFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf>& ethernetWrapper)
    : NetworkManagerFb(CreateType(), ctx, parent, localId, ethernetWrapper)
{
    createFbs();
}

FunctionBlockPtr CaptureModuleFb::create(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
{
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf> ptr = std::make_shared<asam_cmp_common_lib::EthernetPcppImpl>();
    auto fb = createWithImplementation<IFunctionBlock, CaptureModuleFb>(ctx, parent, localId, ptr);
    return fb;
}

FunctionBlockTypePtr CaptureModuleFb::CreateType()
{
    return FunctionBlockType("asam_cmp_capture_module_fb", "AsamCmpCaptureModuleFb", "Asam CMP Capture Module Fb");
}


void CaptureModuleFb::createFbs()
{
    const StringPtr captureModuleId = "asam_cmp_capture_fb";
    auto newFb = createWithImplementation<IFunctionBlock, CaptureFb>(context, functionBlocks, captureModuleId);
    functionBlocks.addItem(newFb);
}

void CaptureModuleFb::networkAdapterChangedInternal()
{

}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
