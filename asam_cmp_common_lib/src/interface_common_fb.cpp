#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

#include <asam_cmp_common_lib/interface_common_fb.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

InterfaceCommonFb::InterfaceCommonFb(const ContextPtr& ctx,
                                             const ComponentPtr& parent,
                                             const StringPtr& localId,
                                             const AsamCmpInterfaceCommonInit& init)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , interfaceIdManager(init.interfaceIdManager)
    , streamIdManager(init.streamIdManager)
    , id(init.id)
    , payloadType(0)
{
    initProperties();
}

FunctionBlockTypePtr InterfaceCommonFb::CreateType()
{
    return FunctionBlockType("asam_cmp_interface", "AsamCmpInterface", "Asam CMP Interface");
}

void InterfaceCommonFb::initProperties()
{
    StringPtr propName = "InterfaceId";
    auto prop = IntPropertyBuilder(propName, id).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateInterfaceIdInternal(); };

    propName = "PayloadType";
    ListPtr<StringPtr> payloadTypes{"UNDEFINED", "CAN", "CAN_FD", "ANALOG"};
    prop = SelectionPropertyBuilder(propName, payloadTypes, 0).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updatePayloadTypeInternal(); };

    propName = "AddStream";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>())).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, Procedure([this] { addStreamInternal(); }));

    propName = "RemoveStream";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("nInd", ctInt)))).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName,
                                                                       Procedure([this](IntPtr nInd) { removeStreamInternal(nInd); }));
}

void InterfaceCommonFb::updateInterfaceIdInternal()
{
    Int newId = objPtr.getPropertyValue("InterfaceId");

    if (newId == id)
        return;

    if (interfaceIdManager->isValidId(newId))
    {
        interfaceIdManager->removeId(id);
        id = newId;
        interfaceIdManager->addId(id);
    }
    else
    {
        objPtr.setPropertyValue("InterfaceId", id);
    }
}

void InterfaceCommonFb::updatePayloadTypeInternal()
{
    Int newType = objPtr.getPropertyValue("PayloadType");

    if (newType < 0 || static_cast<size_t>(newType) > payloadTypeToAsamPayloadType.size())
    {
        objPtr.setPropertyValue("PayloadType", asamPayloadTypeToPayloadType.at(payloadType.getRawPayloadType()));
    }
    else
    {
        payloadType.setRawPayloadType(payloadTypeToAsamPayloadType.at(newType));
    }
}

void InterfaceCommonFb::removeStreamInternal(size_t nInd)
{
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(nInd));
}

END_NAMESPACE_ASAM_CMP_COMMON
