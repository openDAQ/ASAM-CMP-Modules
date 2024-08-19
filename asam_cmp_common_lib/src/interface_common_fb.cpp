#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

#include <asam_cmp_common_lib/interface_common_fb.h>
#include <asam_cmp_common_lib/stream_common_fb_impl.h>
#include <fmt/core.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

InterfaceCommonFb::InterfaceCommonFb(const ContextPtr& ctx,
                                     const ComponentPtr& parent,
                                     const StringPtr& localId,
                                     const InterfaceCommonInit& init)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , interfaceIdManager(init.interfaceIdManager)
    , streamIdManager(init.streamIdManager)
    , interfaceId(init.id)
    , payloadType(0)
    , isUpdating(false)
    , needsPropertyChanged(false)
    , createdStreams(0)
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
    auto prop = IntPropertyBuilder(propName, interfaceId).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChangedIfNotUpdating(); };

    propName = "PayloadType";
    ListPtr<StringPtr> payloadTypes{"UNDEFINED", "CAN", "CAN_FD", "ANALOG"};
    prop = SelectionPropertyBuilder(propName, payloadTypes, 0).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChangedIfNotUpdating(); };

    propName = "AddStream";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>())).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, Procedure([this] { addStream(); }));

    propName = "RemoveStream";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("nInd", ctInt)))).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName,
                                                                       Procedure([this](IntPtr nInd) { removeStream(nInd); }));
}

void InterfaceCommonFb::addStream()
{
    std::scoped_lock lock{sync};
    addStreamInternal();
}

void InterfaceCommonFb::removeStream(size_t nInd)
{
    std::scoped_lock lock{sync};
    removeStreamInternal(nInd);
}

void InterfaceCommonFb::updateInterfaceIdInternal()
{
    Int newId = objPtr.getPropertyValue("InterfaceId");

    if (newId == interfaceId)
        return;

    if (interfaceIdManager->isValidId(newId))
    {
        interfaceIdManager->removeId(interfaceId);
        interfaceId = newId;
        interfaceIdManager->addId(interfaceId);
    }
    else
    {
        setPropertyValueInternal(String("InterfaceId").asPtr<IString>(true),
                                 BaseObjectPtr(interfaceId).asPtr<IBaseObject>(true),
                                 false,
                                 false,
                                 false);
    }
}

void InterfaceCommonFb::updatePayloadTypeInternal()
{
    Int newType = objPtr.getPropertyValue("PayloadType");

    if (newType < 0 || static_cast<size_t>(newType) > payloadTypeToAsamPayloadType.size())
    {
        setPropertyValueInternal(String("PayloadType").asPtr<IString>(true),
                                 BaseObjectPtr(asamPayloadTypeToPayloadType.at(payloadType.getType())).asPtr<IBaseObject>(true),
                                 false,
                                 false,
                                 false);
    }
    else
    {
        payloadType.setType(payloadTypeToAsamPayloadType.at(newType));
        for (const auto& fb : functionBlocks.getItems())
        {
            fb.as<IStreamCommon>(true)->setPayloadType(payloadType);
        }
    }
}

void InterfaceCommonFb::removeStreamInternal(size_t nInd)
{
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(nInd));
}

daq::ErrCode INTERFACE_FUNC InterfaceCommonFb::beginUpdate()
{
    daq::ErrCode result = FunctionBlock::beginUpdate();
    if (result == OPENDAQ_SUCCESS)
        isUpdating = true;

    return result;
}

void InterfaceCommonFb::endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating)
{
    std::scoped_lock lock{sync};
    FunctionBlock::endApplyProperties(propsAndValues, parentUpdating);

    if (needsPropertyChanged)
    {
        propertyChanged();
        needsPropertyChanged = false;
    }

    isUpdating = false;
}

void InterfaceCommonFb::propertyChanged()
{
    updateInterfaceIdInternal();
    updatePayloadTypeInternal();
}

void InterfaceCommonFb::propertyChangedIfNotUpdating()
{
    if (!isUpdating)
    {
        std::scoped_lock lock{sync};
        propertyChanged();
    }
    else
        needsPropertyChanged = true;
}

END_NAMESPACE_ASAM_CMP_COMMON
