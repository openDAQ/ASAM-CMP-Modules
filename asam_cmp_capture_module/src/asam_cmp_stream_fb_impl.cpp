#include <asam_cmp_capture_module/asam_cmp_stream_fb_impl.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

AsamCmpStreamFbImpl::AsamCmpStreamFbImpl(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const AsamCmpStreamCommonInit& init)
    : StreamCommonFbImpl(ctx, parent, localId, init)
{
    encoder = &(*encoders)[id];
    createInputPort();
}

void AsamCmpStreamFbImpl::createInputPort()
{
    inputPort = createAndAddInputPort("input", PacketReadyNotification::Scheduler);
}

void AsamCmpStreamFbImpl::updateStreamIdInternal()
{
    StreamCommonFbImpl::updateStreamIdInternal();

    encoder = &(*encoders)[id];
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
