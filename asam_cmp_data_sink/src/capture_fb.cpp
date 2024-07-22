#include <asam_cmp_data_sink/capture_fb.h>
#include <asam_cmp_data_sink/interface_fb.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <fmt/format.h>
#include <set>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

CaptureModuleFb::CaptureModuleFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : CaptureCommonFb(ctx, parent, localId)
{
}

CaptureModuleFb::CaptureModuleFb(const ContextPtr& ctx,
                                     const ComponentPtr& parent,
                                     const StringPtr& localId,
                                     ASAM::CMP::DeviceStatus&& deviceStatus)
    : CaptureCommonFb(ctx, parent, localId)
    , deviceStatus(std::move(deviceStatus))
{
    createFbs();
}

void CaptureModuleFb::addInterfaceInternal()
{
    auto newId = interfaceIdManager.getFirstUnusedId();
    addInterfaceWithParams<InterfaceFb>(newId);
}

void CaptureModuleFb::createFbs()
{
    for (size_t i = 0; i < deviceStatus.getInterfaceStatusCount(); ++i)
    {
        auto ifStatus = deviceStatus.getInterfaceStatus(i);
        auto newId = ifStatus.getInterfaceId();
        addInterfaceWithParams<InterfaceFb>(newId, std::move(ifStatus));
    }
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
