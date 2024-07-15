#include <asam_cmp_capture_module/asam_cmp_stream_fb_impl.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

AsamCmpStreamFbImpl::AsamCmpStreamFbImpl(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const AsamCmpStreamInit& init)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , streamIdManager(init.streamIdManager)
    , id(init.id)
    , payloadType(init.payloadType)
{
    initProperties();
    createInputPort();
}

FunctionBlockTypePtr AsamCmpStreamFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_stream", "AsamCmpStream", "Asam CMP Stream");
}

void AsamCmpStreamFbImpl::initProperties()
{
    StringPtr propName = "StreamId";
    auto prop = IntPropertyBuilder(propName, id).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateStreamIdInternal(); };
}

void AsamCmpStreamFbImpl::createInputPort()
{
    inputPort = createAndAddInputPort("input", PacketReadyNotification::Scheduler);
}

void AsamCmpStreamFbImpl::updateStreamIdInternal()
{
    Int newId = objPtr.getPropertyValue("InterfaceId");

    if (streamIdManager->isValidId(newId))
    {
        streamIdManager->removeId(id);
        id = newId;
        streamIdManager->addId(id);
    }
    else
    {
        objPtr.setPropertyValue("InterfaceId", id);
    }
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
