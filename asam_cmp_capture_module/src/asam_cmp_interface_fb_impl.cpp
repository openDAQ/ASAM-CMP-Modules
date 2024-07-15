#include <asam_cmp_capture_module/asam_cmp_interface_fb_impl.h>
#include <asam_cmp_capture_module/asam_cmp_stream_fb_impl.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coretypes/listobject_factory.h>

#include <iostream>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

AsamCmpInterfaceFbImpl::AsamCmpInterfaceFbImpl(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const AsamCmpInterfaceInit& init)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , interfaceIdManager(init.interfaceIdManager)
    , streamIdManager(init.streamIdManager)
    , id(init.id)
    , payloadType(0)
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
    auto prop = IntPropertyBuilder(propName, id).build();
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
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updatePayloadTypeInternal(); };

    propName = "AddStream";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>())).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, Procedure([this] { this->addStreamInternal(); }));

    propName = "RemoveStream";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("nInd", ctInt)))).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName,
                                                                       Procedure([this](IntPtr nInd) { removeStreamInternal(nInd); }));
}

void AsamCmpInterfaceFbImpl::updateInterfaceIdInternal()
{
    Int newId = objPtr.getPropertyValue("InterfaceId");

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

void AsamCmpInterfaceFbImpl::updatePayloadTypeInternal()
{
    Int newType = objPtr.getPropertyValue("PayloadType");

    if (newType < 0 || newType > payloadTypeToAsamPayloadType.size())
    {
        objPtr.setPropertyValue("PayloadType", asamPayloadTypeToPayloadType.at(payloadType.getRawPayloadType()));
    }
    else
    {
        payloadType.setRawPayloadType(payloadTypeToAsamPayloadType.at(newType));
    }
}

void AsamCmpInterfaceFbImpl::addStreamInternal()
{
    std::cout << objPtr.getPropertyValue("PayloadType") << std::endl;
    AsamCmpStreamInit init{createdStreams,
                           payloadType,
                           streamIdManager};

    StringPtr fbId = fmt::format("asam_cmp_stream_{}", createdStreams++);
    functionBlocks.addItem(createWithImplementation<IFunctionBlock, AsamCmpStreamFbImpl>(context, functionBlocks, fbId, init));
}

void AsamCmpInterfaceFbImpl::removeStreamInternal(size_t nInd)
{
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(nInd));
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
