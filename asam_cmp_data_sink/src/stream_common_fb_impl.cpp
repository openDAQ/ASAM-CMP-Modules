#include <asam_cmp_data_sink/stream_common_fb_impl.h>

#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

StreamCommonFbImpl::StreamCommonFbImpl(const ContextPtr& ctx,
                                       const ComponentPtr& parent,
                                       const StringPtr& localId,
                                       const AsamCmpStreamCommonInit& init)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , streamIdManager(init.streamIdManager)
    , id(init.id)
    , payloadType(init.payloadType)
{
    initProperties();
}

FunctionBlockTypePtr StreamCommonFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_stream", "AsamCmpStream", "Asam CMP Stream");
}

void StreamCommonFbImpl::initProperties()
{
    StringPtr propName = "StreamId";
    auto prop = IntPropertyBuilder(propName, id).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateStreamIdInternal(); };
}

void StreamCommonFbImpl::updateStreamIdInternal()
{
    Int newId = objPtr.getPropertyValue("InterfaceId");

    if (newId == id)
        return;

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

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
