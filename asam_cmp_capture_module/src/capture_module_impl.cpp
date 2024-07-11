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
    AsamCmpInterfaceInit init{interfacesCreated, FunctionPtr([this](uint32_t id) { return this->isInterfaceIdUnique(id); })};

    StringPtr fbId = fmt::format("asam_cmp_interface_{}", interfacesCreated++);
    functionBlocks.addItem(createWithImplementation<IFunctionBlock, AsamCmpInterfaceFbImpl>(context, functionBlocks, fbId, init));
}

void CaptureModuleImpl::removeInterfaceInternal(size_t nInd)
{
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(nInd));
}

bool CaptureModuleImpl::isInterfaceIdUnique(size_t id)
{
    int cnt = 0;
    for (const auto& itf : functionBlocks.getItems())
        cnt += (itf.getPropertyValue("InterfaceId") == id);

    return cnt == 1;
}
END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
