#include <asam_cmp_capture_module/asam_cmp_interface_fb_impl.h>
#include <asam_cmp_capture_module/asam_cmp_stream_fb_impl.h>

#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

#include <iostream>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

AsamCmpInterfaceFbImpl::AsamCmpInterfaceFbImpl(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const AsamCmpInterfaceCommonInit& init,
                                               AsamCmpEncoderBankPtr encoders)
    : InterfaceCommonFbImpl(ctx, parent, localId, init)
    , encoders(encoders)
{
}

void AsamCmpInterfaceFbImpl::addStreamInternal()
{
    addStreamWithParams<AsamCmpStreamFbImpl>();
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
