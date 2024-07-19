#include <asam_cmp_data_sink/capture_module_impl.h>
#include <asam_cmp_data_sink/interface_fb_impl.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <fmt/format.h>
#include <set>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

CaptureModuleImpl::CaptureModuleImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : CaptureModuleCommonImpl(ctx, parent, localId)
{
}

CaptureModuleImpl::CaptureModuleImpl(const ContextPtr& ctx,
                                     const ComponentPtr& parent,
                                     const StringPtr& localId,
                                     ASAM::CMP::DeviceStatus&& deviceStatus)
    : CaptureModuleCommonImpl(ctx, parent, localId)
    , deviceStatus(std::move(deviceStatus))
{
    createFbs();
}

void CaptureModuleImpl::addInterfaceInternal()
{
    auto newId = interfaceIdManager.getFirstUnusedId();
    addInterfaceWithParams<InterfaceFbImpl>(newId);
}

void CaptureModuleImpl::createFbs()
{
    for (size_t i = 0; i < deviceStatus.getInterfaceStatusCount(); ++i)
    {
        auto ifStatus = deviceStatus.getInterfaceStatus(i);
        auto newId = ifStatus.getInterfaceId();
        addInterfaceWithParams<InterfaceFbImpl>(newId, std::move(ifStatus));
    }
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
