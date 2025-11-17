#include <opendaq/component_type_private.h>

#include <asam_cmp_capture_module/capture_fb.h>
#include <asam_cmp_capture_module/capture_module_fb.h>
#include <asam_cmp_common_lib/ethernet_pcpp_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

CaptureModuleFb::CaptureModuleFb(const ModuleInfoPtr& moduleInfo,const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf>& ethernetWrapper)
    : NetworkManagerFb(CreateType(moduleInfo), ctx, parent, localId, ethernetWrapper)
{
    createFbs();
}

FunctionBlockPtr CaptureModuleFb::create(const ModuleInfoPtr& moduleInfo, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
{
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf> ptr = std::make_shared<asam_cmp_common_lib::EthernetPcppImpl>();
    auto fb = createWithImplementation<IFunctionBlock, CaptureModuleFb>(moduleInfo, ctx, parent, localId, ptr);
    return fb;
}

FunctionBlockTypePtr CaptureModuleFb::CreateType(const ModuleInfoPtr& moduleInfo)
{
    auto fbType = FunctionBlockType("AsamCmpCaptureModule", "AsamCmpCaptureModule", "ASAM CMP Capture Module");

    checkErrorInfo(fbType.asPtr<IComponentTypePrivate>(true)->setModuleInfo(moduleInfo));
    return fbType;
}

void CaptureModuleFb::createFbs()
{
    const StringPtr captureModuleId = "Capture";
    CaptureFbInit init{ethernetWrapper, selectedEthernetDeviceName};
    auto newFb = createWithImplementation<IFunctionBlock, CaptureFb>(
        this->type.getModuleInfo(), context, functionBlocks, captureModuleId, init);
    newFb.setName("Capture");
    functionBlocks.addItem(newFb);
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
