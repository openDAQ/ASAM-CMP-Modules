#include <asam_cmp_capture_module/stream_fb_impl.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

StreamFbImpl::StreamFbImpl(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const StreamInit& init)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , streamIdManager(init.streamIdManager)
    , id(init.id)
    , payloadType(init.payloadType)
{
    initProperties();
    encoder = &(*encoders)[id];
    createInputPort();
}

FunctionBlockTypePtr StreamFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_stream", "AsamCmpStream", "Asam CMP Stream");
}

void StreamFbImpl::initProperties()
{
    StringPtr propName = "StreamId";
    auto prop = IntPropertyBuilder(propName, id).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateStreamIdInternal(); };
}

void StreamFbImpl::createInputPort()
{
    inputPort = createAndAddInputPort("input", PacketReadyNotification::Scheduler);
}

void StreamFbImpl::updateStreamIdInternal()
{
    Int newId = objPtr.getPropertyValue("InterfaceId");

    if (newId == id)
        return;

    if (streamIdManager->isValidId(newId))
    {
        streamIdManager->removeId(id);
        id = newId;
        encoder = &(*encoders)[id];
        streamIdManager->addId(id);
    }
    else
    {
        objPtr.setPropertyValue("InterfaceId", id);
    }
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
