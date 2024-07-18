#include <asam_cmp/capture_module_payload.h>
#include <coreobjects/callable_info_factory.h>

#include <asam_cmp_data_sink/asam_cmp_status_fb_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

AsamCmpStatusFbImpl::AsamCmpStatusFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlockImpl(CreateType(), ctx, parent, localId)
{
    initProperties();
}

FunctionBlockTypePtr AsamCmpStatusFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_status", "AsamCmpStatus", "ASAM CMP Status");
}

void AsamCmpStatusFbImpl::processStatusPacket(const std::shared_ptr<ASAM::CMP::Packet>& packet)
{
    std::scoped_lock lock(stMutex, sync);

    status.update(*packet);

    size_t index = status.getIndexByDeviceId(packet->getDeviceId());
    // If an Interface status packet came before a Capture status packet
    if (index >= status.getDeviceStatusCount())
        return;

    const auto& deviceStatus = status.getDeviceStatus(index);
    const auto& payload = static_cast<const ASAM::CMP::CaptureModulePayload&>(deviceStatus.getPacket().getPayload());
    StringPtr newLine = fmt::format("Id: {}, Name: {}, Interfaces: {}",
                                    deviceStatus.getPacket().getDeviceId(),
                                    payload.getDeviceDescription(),
                                    deviceStatus.getInterfaceStatusCount());

    ListPtr<IString> devices = objPtr.getPropertyValue("CaptureModuleList");
    if (index < devices.getCount())
        devices.setItemAt(index, newLine);
    else
        devices.pushBack(newLine);

    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("CaptureModuleList", devices);
}

StatusMt AsamCmpStatusFbImpl::getStatusMt() const
{
    return StatusMt(status, stMutex);
}

void AsamCmpStatusFbImpl::initProperties()
{
    StringPtr propName = "CaptureModuleList";
    auto list = List<IString>();
    auto prop = ListPropertyBuilder(propName, list).setReadOnly(true).build();
    objPtr.addProperty(prop);

    propName = "Clear";
    objPtr.addProperty(FunctionPropertyBuilder(propName, ProcedureInfo()).setReadOnly(true).build());
    auto proc = Procedure([this](IntPtr nItem) { clear(); });
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, proc);
}

void AsamCmpStatusFbImpl::clear()
{
    std::scoped_lock lock{stMutex, sync};

    status.clear();
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("CaptureModuleList", List<IString>());
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
