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
    , ethernetWrapper(init.ethernetWrapper)
    , selectedEthernetDeviceName(init.selectedDeviceName)
    , deviceDescription("DefaultDeviceDescription")
    , serialNumber("DefaultSerailNumber")
    , hardwareVersion("DefaultHardwwareVersion")
    , softwareVersion("DefaultSoftwareVersion")
    , vendorDataAsString("")
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
    StringPtr propName = "DeviceDescription";
    auto prop = StringPropertyBuilder(propName, deviceDescription).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateCaptureData(); };

    propName = "SerialNumber";
    prop = StringPropertyBuilder(propName, serialNumber).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateCaptureData(); };

    propName = "HardwareVersion";
    prop = StringPropertyBuilder(propName, hardwareVersion).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateCaptureData(); };

    propName = "SoftwareVersion";
    prop = StringPropertyBuilder(propName, softwareVersion).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateCaptureData(); };

    propName = "VendorData";
    prop = StringPropertyBuilder(propName, vendorDataAsString).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateCaptureData(); };
}

void CaptureFb::updateCaptureData()
{
    std::scoped_lock lock(statusSync);

    deviceDescription = objPtr.getPropertyValue("DeviceDescription");
    serialNumber = objPtr.getPropertyValue("SerialNumber");
    hardwareVersion = objPtr.getPropertyValue("HardwareVersion");
    softwareVersion = objPtr.getPropertyValue("SoftwareVersion");
    vendorDataAsString = objPtr.getPropertyValue("VendorData").asPtr<IString>().toStdString();
    vendorData = std::vector<uint8_t>(begin(vendorDataAsString), end(vendorDataAsString));


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
    for (int i = 0; i < encoders.size(); ++i)
    {
        encoders[i].setDeviceId(deviceId);
        encoders[i].setStreamId(i);
    }
}

void CaptureFb::initStatusPacket()
{
    captureStatusPacket.setPayload(ASAM::CMP::CaptureModulePayload());
    captureStatusPacket.getPayload().setMessageType(ASAM::CMP::CmpHeader::MessageType::status);
    updateCaptureData();
}

void CaptureFb::addInterfaceInternal(){
    auto newId = interfaceIdManager.getFirstUnusedId();
    InterfaceFbInit init{&encoders, captureStatus, statusSync};
    addInterfaceWithParams<InterfaceFb>(newId, init);
}

void CaptureFb::statusLoop()
{
    std::unique_lock<std::mutex> lock(statusSync);
    while (!stopStatusSending)
    {
        cv.wait_for(lock, std::chrono::milliseconds(sendingSyncLoopTime));
        if (!stopStatusSending)
        {
            auto encodeAndSend = [&](const ASAM::CMP::Packet& packet) {
                auto encodedData = encoders[1].encode(packet, {64, 1500});  // TODO: magic numbers
                for (const auto& e : encodedData)
                    ethernetWrapper->sendPacket(selectedEthernetDeviceName, e);
            };


            auto encodedData = encoders[1].encode(captureStatus.getPacket(), {64, 1500});  // TODO: magic numbers
            for (const auto& e : encodedData)
                ethernetWrapper->sendPacket(selectedEthernetDeviceName, e);

           
            for (int i = 0; i < captureStatus.getInterfaceStatusCount(); ++i)
            {
                encodedData = encoders[1].encode(captureStatus.getInterfaceStatus(i).getPacket(), {64, 1500});  // TODO: magic numbers
                for (const auto& e : encodedData)
                    ethernetWrapper->sendPacket(selectedEthernetDeviceName, e);
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
        std::scoped_lock<std::mutex> lock(sync);
        stopStatusSending = true;
    }
    cv.notify_one();

    statusThread.join();
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
