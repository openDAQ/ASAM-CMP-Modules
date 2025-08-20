#include <asam_cmp/capture_module_payload.h>

#include <asam_cmp_data_sink/capture_fb.h>
#include <asam_cmp_data_sink/interface_fb.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

CaptureFb::CaptureFb(const ContextPtr& ctx,
                     const ComponentPtr& parent,
                     const StringPtr& localId,
                     DataPacketsPublisher& dataPacketsPublisher,
                     CapturePacketsPublisher& capturePacketsPublisher)
    : CaptureCommonFbImpl(ctx, parent, localId)
    , dataPacketsPublisher(dataPacketsPublisher)
    , capturePacketsPublisher(capturePacketsPublisher)
{
    initDeviceInfoProperties(true);
}

CaptureFb::CaptureFb(const ContextPtr& ctx,
                     const ComponentPtr& parent,
                     const StringPtr& localId,
                     DataPacketsPublisher& dataPacketsPublisher,
                     CapturePacketsPublisher& capturePacketsPublisher,
                     ASAM::CMP::DeviceStatus&& deviceStatus)
    : CaptureCommonFbImpl(ctx, parent, localId)
    , deviceStatus(std::move(deviceStatus))
    , dataPacketsPublisher(dataPacketsPublisher)
    , capturePacketsPublisher(capturePacketsPublisher)
{
    initDeviceInfoProperties(true);
    setProperties();
    createFbs();
}

void CaptureFb::receive(const std::shared_ptr<ASAM::CMP::Packet>& packet)
{
    setDeviceInfoProperties(*packet);
}

void CaptureFb::updateDeviceIdInternal()
{
    auto oldDeviceId = deviceId;
    CaptureCommonFbImpl::updateDeviceIdInternal();
    if (oldDeviceId == deviceId)
        return;

    capturePacketsPublisher.unsubscribe(oldDeviceId, this);
    capturePacketsPublisher.subscribe(deviceId, this);

    for (const FunctionBlockPtr& interfaceFb : functionBlocks.getItems())
    {
        uint32_t interfaceId = interfaceFb.getPropertyValue("InterfaceId");
        for (const auto& streamFb : interfaceFb.getFunctionBlocks())
        {
            uint8_t streamId = static_cast<Int>(streamFb.getPropertyValue("StreamId"));
            auto handler = streamFb.as<IAsamCmpPacketsSubscriber>(true);
            dataPacketsPublisher.unsubscribe({oldDeviceId, interfaceId, streamId}, handler);
            dataPacketsPublisher.subscribe({deviceId, interfaceId, streamId}, handler);
        }
    }
}

void CaptureFb::addInterfaceInternal()
{
    auto interfaceId = interfaceIdManager.getFirstUnusedId();
    addInterfaceWithParams<InterfaceFb>(interfaceId, deviceId, dataPacketsPublisher);
}

void CaptureFb::removeInterfaceInternal(size_t nInd)
{
    FunctionBlockPtr interfaceFb = functionBlocks.getItems().getItemAt(nInd);
    uint32_t interfaceId = interfaceFb.getPropertyValue("InterfaceId");
    for (const auto& streamFb : interfaceFb.getFunctionBlocks())
    {
        uint8_t streamId = static_cast<Int>(streamFb.getPropertyValue("StreamId"));
        auto handler = streamFb.as<IAsamCmpPacketsSubscriber>(true);
        dataPacketsPublisher.unsubscribe({deviceId, interfaceId, streamId}, handler);
    }

    CaptureCommonFbImpl::removeInterfaceInternal(nInd);
}

void CaptureFb::setProperties()
{
    objPtr.setPropertyValue("DeviceId", deviceStatus.getPacket().getDeviceId());

    setDeviceInfoProperties(deviceStatus.getPacket());
}

void CaptureFb::setDeviceInfoProperties(const ASAM::CMP::Packet& packet)
{
    auto& cmPayload = static_cast<const ASAM::CMP::CaptureModulePayload&>(packet.getPayload());

    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("DeviceDescription", cmPayload.getDeviceDescription().data());
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("SerialNumber", cmPayload.getSerialNumber().data());
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("HardwareVersion", cmPayload.getHardwareVersion().data());
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("SoftwareVersion", cmPayload.getSoftwareVersion().data());
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("VendorData", std::string(cmPayload.getVendorDataStringView()));
}

void CaptureFb::createFbs()
{
    for (size_t i = 0; i < deviceStatus.getInterfaceStatusCount(); ++i)
    {
        auto ifStatus = deviceStatus.getInterfaceStatus(i);
        auto interfaceId = ifStatus.getInterfaceId();
        addInterfaceWithParams<InterfaceFb>(interfaceId, deviceId, dataPacketsPublisher, std::move(ifStatus));
    }
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
