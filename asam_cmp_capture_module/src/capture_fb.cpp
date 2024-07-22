#include <asam_cmp_capture_module/capture_fb.h>
#include <asam_cmp_capture_module/interface_fb.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <set>
#include <fmt/format.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

CaptureFb::CaptureFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : asam_cmp_common_lib::CaptureCommonFb(ctx, parent, localId)
{
    initEncoders();
}

void CaptureFb::initEncoders()
{
    for (int i = 0; i < encoders.size(); ++i)
    {
        encoders[i].setDeviceId(deviceId);
        encoders[i].setStreamId(i);
    }
}

void CaptureFb::addInterfaceInternal(){
    auto newId = interfaceIdManager.getFirstUnusedId();
    addInterfaceWithParams<InterfaceFb>(newId);
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
