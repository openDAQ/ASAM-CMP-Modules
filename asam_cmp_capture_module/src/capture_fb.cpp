#include <asam_cmp_capture_module/capture_fb.h>
#include <asam_cmp_capture_module/interface_fb.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <set>
#include <fmt/format.h>
#include <asam_cmp/cmp_header.h>
#include <asam_cmp_common_lib/ethernet_pcpp_itf.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

CaptureFb::CaptureFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const CaptureFbInit& init)
    : asam_cmp_common_lib::CaptureCommonFb(ctx, parent, localId)
    , allowJumboFrames(false)
    , ethernetWrapper(init.ethernetWrapper)
    , selectedEthernetDeviceName(init.selectedDeviceName)
{
    initProperties();
    initEncoders();
    initStatusPacket();
    startStatusLoop();
}

CaptureFb::~CaptureFb()
{
    stopStatusLoop();
}

void CaptureFb::initProperties()
{
    initDeviceInfoProperties(false);
    deviceDescription = "DefaultDeviceDescription";
    setPropertyValueInternal(String("DeviceDescription").asPtr<IString>(true), deviceDescription, false, false, false);
    serialNumber = "DefaultSerialNumber";
    setPropertyValueInternal(String("SerialNumber").asPtr<IString>(true), serialNumber, false, false, false);
    hardwareVersion = "DefaultHardwareVersion";
    setPropertyValueInternal(String("HardwareVersion").asPtr<IString>(true), hardwareVersion, false, false, false);
    softwareVersion = "DefaultSoftwareVersion";
    setPropertyValueInternal(String("SoftwareVersion").asPtr<IString>(true), softwareVersion, false, false, false);
}

void CaptureFb::propertyChanged()
{
    asam_cmp_common_lib::CaptureCommonFb::propertyChanged();

    initEncoders();
    updateCaptureData();
}

void CaptureFb::updateCaptureData()
{
    std::scoped_lock lock{statusSync};

    captureStatusPacket.setDeviceId(deviceId);
    static_cast<ASAM::CMP::CaptureModulePayload&>(captureStatusPacket.getPayload())
        .setData(
            deviceDescription.toView(),
            serialNumber.toView(),
            hardwareVersion.toView(),
            softwareVersion.toView(),
            vendorData
        );

    captureStatus.update(captureStatusPacket);
}

void CaptureFb::initEncoders()
{
    encoders.init(deviceId);
}

void CaptureFb::initStatusPacket()
{
    captureStatusPacket.setPayload(ASAM::CMP::CaptureModulePayload());
    captureStatusPacket.getPayload().setMessageType(ASAM::CMP::CmpHeader::MessageType::status);
    updateCaptureData();
}

void CaptureFb::addInterfaceInternal(){
    std::scoped_lock lock{statusSync};

    auto newId = interfaceIdManager.getFirstUnusedId();
    InterfaceFbInit init{&encoders, captureStatus, statusSync, ethernetWrapper, allowJumboFrames, selectedEthernetDeviceName};
    addInterfaceWithParams<InterfaceFb>(newId, init);
}

void CaptureFb::removeInterfaceInternal(size_t nInd)
{
    std::scoped_lock lock{statusSync};

    int id = functionBlocks.getItems().getItemAt(nInd).getPropertyValue("InterfaceId");
    captureStatus.removeInterfaceById(id);
    asam_cmp_common_lib::CaptureCommonFb::removeInterfaceInternal(nInd);
}

ASAM::CMP::DataContext CaptureFb::createEncoderDataContext() const
{
    assert(!allowJumboFrames);
    return {64, 1500};
}

void CaptureFb::statusLoop()
{
    auto encoderContext = createEncoderDataContext();
    std::unique_lock<std::mutex> lock(statusSync);
    while (!stopStatusSending)
    {
        cv.wait_for(lock, std::chrono::milliseconds(sendingSyncLoopTime));
        if (!stopStatusSending)
        {
            auto encodeAndSend = [&](const ASAM::CMP::Packet& packet) {
                auto encodedData = encoders.encode(1, packet, encoderContext);
                for (const auto& e : encodedData)
                    ethernetWrapper->sendPacket(e);
            };


            auto encodedData = encoders.encode(1, captureStatus.getPacket(), encoderContext);
            for (const auto& e : encodedData)
                ethernetWrapper->sendPacket(e);

           
            for (SizeT i = 0; i < captureStatus.getInterfaceStatusCount(); ++i)
            {
                encodedData = encoders.encode(1, captureStatus.getInterfaceStatus(i).getPacket(), encoderContext);
                for (const auto& e : encodedData)
                    ethernetWrapper->sendPacket(e);
            }
        }
    }
}

void CaptureFb::startStatusLoop()
{
    stopStatusSending = false;
    statusThread = std::thread{&CaptureFb::statusLoop, this};
}

void CaptureFb::stopStatusLoop()
{
    {
        auto lock2 = getRecursiveConfigLock();
        stopStatusSending = true;
    }
    cv.notify_one();

    statusThread.join();
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
