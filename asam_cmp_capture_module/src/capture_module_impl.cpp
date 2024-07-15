#include <asam_cmp_capture_module/capture_module_impl.h>
#include <asam_cmp_capture_module/asam_cmp_interface_fb_impl.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <set>
#include <fmt/format.h>

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
    StringPtr propName = "DeviceId";
    auto prop = IntPropertyBuilder(propName, 0).build();
    objPtr.addProperty(prop);

    propName = "AddInterface";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>())).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, Procedure([this] { this->addInterfaceInternal(); }));

    propName = "RemoveInterface";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("nInd", ctInt)))).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName,
                                                                       Procedure([this](IntPtr nInd) { removeInterfaceInternal(nInd); }));
}

void CaptureModuleImpl::addInterfaceInternal(){
    auto newId = interfaceIdManager.getFirstUnusedId();
    AsamCmpInterfaceInit init{newId, &interfaceIdManager, &streamIdManager};

    StringPtr fbId = fmt::format("asam_cmp_interface_{}", newId);
    functionBlocks.addItem(createWithImplementation<IFunctionBlock, AsamCmpInterfaceFbImpl>(context, functionBlocks, fbId, init));
    interfaceIdManager.addId(newId);
}

void CaptureModuleImpl::removeInterfaceInternal(size_t nInd)
{
    interfaceIdManager.removeId(functionBlocks.getItems().getItemAt(nInd).getPropertyValue("InterfaceId"));
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(nInd));
}
END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
