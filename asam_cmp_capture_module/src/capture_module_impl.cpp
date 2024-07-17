#include <asam_cmp_capture_module/asam_cmp_interface_fb_impl.h>
#include <asam_cmp_capture_module/capture_module_impl.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <fmt/format.h>
#include <set>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

CaptureModuleImpl::CaptureModuleImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : CaptureModuleCommonImpl(ctx, parent, localId)
{
    initEncoders();
}

void CaptureModuleImpl::initEncoders()
{
    for (int i = 0; i < encoders.size(); ++i)
    {
        encoders[i].setDeviceId(deviceId);
        encoders[i].setStreamId(i);
    }
}

void CaptureModuleImpl::updateDeviceId()
{
    CaptureModuleCommonImpl::updateDeviceId();
    initEncoders();
}

void CaptureModuleImpl::addInterfaceInternal()
{
    addInterfaceWithParams<AsamCmpInterfaceFbImpl>(&encoders);
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
