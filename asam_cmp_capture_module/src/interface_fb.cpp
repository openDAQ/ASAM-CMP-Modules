#include <asam_cmp_capture_module/interface_fb.h>
#include <asam_cmp_capture_module/stream_fb.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coretypes/listobject_factory.h>

#include <iostream>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

InterfaceFb::InterfaceFb(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const asam_cmp_common_lib::InterfaceCommonInit& init,
                                               const EncoderBankPtr& encoders)
    : InterfaceCommonFb(ctx, parent, localId, init)
    , encoders(encoders)
{
}

void InterfaceFb::addStreamInternal()
{
    auto newId = streamIdManager->getFirstUnusedId();
    addStreamWithParams<StreamFb>(newId);
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
