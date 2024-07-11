#include <asam_cmp_capture_module/asam_cmp_interface_fb_impl.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

AsamCmpInterfaceFbImpl::AsamCmpInterfaceFbImpl(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const AsamCmpInterfaceInit& init)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , interfaceIdValidator(init.validator)
    , lastId(init.id)
{
    initProperties();
}

FunctionBlockTypePtr AsamCmpInterfaceFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_interface", "AsamCmpInterface", "Asam CMP Interface");
}

void AsamCmpInterfaceFbImpl::initProperties()
{
    StringPtr propName = "InterfaceId";
    auto prop = IntPropertyBuilder(propName, lastId).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateInterfaceIdInternal(); };


    propName = "PayloadType";
    ListPtr<StringPtr> payloadTypes{"UNDEFINED",
                                    "CAN",
                                    "CAN_FD",
                                    "ANALOG"
    };
    prop = SelectionPropertyBuilder(propName, payloadTypes, 0).build();

    propName = "AddStream";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>())).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(
        propName, Procedure([this] { throw DaqException::exception("Not implemented"); }));

    propName = "RemoveStream";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("nInd", ctInt)))).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(
        propName, Procedure([this](int nInd) { throw DaqException::exception("Not implemented"); }));
}

void AsamCmpInterfaceFbImpl::updateInterfaceIdInternal()
{
    Int newId = objPtr.getPropertyValue("InterfaceId");

    if (newId < 0 || newId > std::numeric_limits<uint32_t>::max() || !interfaceIdValidator.call(newId))
    {
        objPtr.setPropertyValue("InterfaceId", lastId);
    }
    else
    {
        lastId = newId;
    }
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
