#include <asam_cmp_capture_module/capture_module_impl.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>

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
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(
        propName, Procedure([this] { throw DaqException::exception("Not implemented"); }));

    propName = "RemoveInterface";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("nInd", ctInt)))).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(
        propName, Procedure([this](int nInd) { throw DaqException::exception("Not implemented"); }));

    //TODO: current proposal is to put a copy of functionBlocks variable every time user add new Interface
    propName = "InterfacesList";
    prop = ListPropertyBuilder(propName, List<FunctionBlockPtr>()).setReadOnly(true).build();
    objPtr.addProperty(prop);
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
