#include <asam_cmp_capture_module/interface_fb_impl.h>
#include <asam_cmp_capture_module/stream_fb_impl.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coretypes/listobject_factory.h>

#include <iostream>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

InterfaceFbImpl::InterfaceFbImpl(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const InterfaceInit& init)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , interfaceIdManager(init.interfaceIdManager)
    , streamIdManager(init.streamIdManager)
    , encoders(init.encoders)
    , id(init.id)
    , payloadType(0)
{
    initProperties();
}

FunctionBlockTypePtr InterfaceFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_interface", "AsamCmpInterface", "Asam CMP Interface");
}

void InterfaceFbImpl::initProperties()
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

void InterfaceFbImpl::updateInterfaceIdInternal()
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

void InterfaceFbImpl::updatePayloadTypeInternal()
{
    Int newType = objPtr.getPropertyValue("PayloadType");

    if (newType < 0 || (size_t)newType > payloadTypeToAsamPayloadType.size())
    {
        objPtr.setPropertyValue("PayloadType", asamPayloadTypeToPayloadType.at(payloadType.getRawPayloadType()));
    }
    else
    {
        payloadType.setRawPayloadType(payloadTypeToAsamPayloadType.at(newType));
    }
}

void InterfaceFbImpl::addStreamInternal()
{
    std::cout << objPtr.getPropertyValue("PayloadType") << std::endl;
    auto id = streamIdManager->getFirstUnusedId();
    StreamInit init{id,
                           payloadType,
                           streamIdManager};

    StringPtr fbId = fmt::format("asam_cmp_stream_{}", id);
    functionBlocks.addItem(createWithImplementation<IFunctionBlock, StreamFbImpl>(context, functionBlocks, fbId, init));
    streamIdManager->addId(id);
}

void InterfaceFbImpl::removeStreamInternal(size_t nInd)
{
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(nInd));
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
