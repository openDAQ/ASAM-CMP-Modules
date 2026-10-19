#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/capture_fb.h>
#include <opendaq/context_factory.h>
#include <opendaq/device_impl.h>
#include <opendaq/folder_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <gtest/gtest.h>

#include <asam_cmp_common_lib/ethernet_pcpp_mock.h>
#include <asam_cmp/decoder.h>

using namespace daq;
using namespace testing;

class CaptureFbParentDevice final : public Device
{
public:
    CaptureFbParentDevice(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const DeviceInfoPtr& info)
        : Device(ctx, parent, localId)
    {
        deviceInfo = info;
    }
};

class CaptureFbTest : public testing::Test
{
protected:
    CaptureFbTest()
    : ethernetWrapper(std::make_shared<asam_cmp_common_lib::EthernetPcppMock>())
    {
        auto startStub = []() {};
        auto stopStub = []() {};

        ON_CALL(*ethernetWrapper, startCapture(_)).WillByDefault(startStub);
        ON_CALL(*ethernetWrapper, stopCapture()).WillByDefault(stopStub);
        ON_CALL(*ethernetWrapper, getEthernetDevicesNamesList()).WillByDefault(Return(names));
        ON_CALL(*ethernetWrapper, getEthernetDevicesDescriptionsList()).WillByDefault(Return(descriptions));
        ON_CALL(*ethernetWrapper, sendPacket(_))
            .WillByDefault(WithArgs<0>(Invoke(
                [&](const std::vector<uint8_t>& data){
            this->onPacketSendCb(data);}
        )));

        auto logger = Logger();
        context = Context(Scheduler(logger), logger, TypeManager(), nullptr, nullptr);
        const StringPtr captureModuleId = "asam_cmp_capture_fb";
        selectedDevice = "device1";
        modules::asam_cmp_capture_module::CaptureFbInit init = {ethernetWrapper, selectedDevice};
        captureFb = createWithImplementation<IFunctionBlock, modules::asam_cmp_capture_module::CaptureFb>(
            moduleInfo, context, nullptr, captureModuleId, init);
    }

protected:
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppMock> ethernetWrapper;
    ListPtr<StringPtr> names;
    ListPtr<StringPtr> descriptions;

    StringPtr selectedDevice;
    ContextPtr context;
    ModuleInfoPtr moduleInfo;
    FunctionBlockPtr captureFb;

    std::mutex packedReceivedSync;
    ASAM::CMP::Packet lastReceivedPacket;
    ASAM::CMP::Decoder decoder;

    void createCaptureWithParent(const ComponentPtr& parent)
    {
        modules::asam_cmp_capture_module::CaptureFbInit init{ethernetWrapper, selectedDevice};
        captureFb = createWithImplementation<IFunctionBlock, modules::asam_cmp_capture_module::CaptureFb>(
            moduleInfo, context, parent, "capture", init);
    }

    void onPacketSendCb(const std::vector<uint8_t>& data)
    {
        std::scoped_lock lock{packedReceivedSync};
        std::cout << "onPacketSend detected\n";
        lastReceivedPacket = *(decoder.decode(data.data(), data.size()).back().get());
    };
};

TEST_F(CaptureFbTest, CreateCaptureModule)
{
    ASSERT_NE(captureFb, nullptr);
}

TEST_F(CaptureFbTest, DefaultPropertiesValues)
{
    constexpr Int defaultDeviceId = 0;
    const StringPtr defaultDefaultDeviceDescription = "";
    const StringPtr defaultDefaultSerialNumber = "";
    const StringPtr defaultDefaultHardwareVersion = "";
    const StringPtr defaultDefaultSoftwareVersion = "";
    const StringPtr defaultDefaultVendorData = "";

    const Int id = captureFb.getPropertyValue("DeviceId");
    const StringPtr deviceDescription = captureFb.getPropertyValue("DeviceDescription");
    const StringPtr serialNumber = captureFb.getPropertyValue("SerialNumber");
    const StringPtr hardwareVersion = captureFb.getPropertyValue("HardwareVersion");
    const StringPtr softwareVersion = captureFb.getPropertyValue("SoftwareVersion");
    const StringPtr vendorData = captureFb.getPropertyValue("VendorData");

    ASSERT_EQ(id, defaultDeviceId);
    ASSERT_EQ(deviceDescription, defaultDefaultDeviceDescription);
    ASSERT_EQ(serialNumber, defaultDefaultSerialNumber);
    ASSERT_EQ(hardwareVersion, defaultDefaultHardwareVersion);
    ASSERT_EQ(softwareVersion, defaultDefaultSoftwareVersion);
    ASSERT_EQ(vendorData, defaultDefaultVendorData);
    for (const auto* name : {"DeviceDescription", "SerialNumber", "HardwareVersion", "SoftwareVersion"})
    {
        ASSERT_FALSE(captureFb.getProperty(name).getReadOnly());
        captureFb.setPropertyValue(name, "manual");
        ASSERT_EQ(captureFb.getPropertyValue(name), String("manual"));
    }
}

TEST_F(CaptureFbTest, InheritDeviceProperties)
{
    auto info = DeviceInfo("test://device", "Measurement system");
    info.setManufacturer("Dewesoft");
    info.setModel("RT Simulator");
    info.setSerialNumber("DS12345677");
    info.setHardwareRevision("9999.1.2345");
    info.setSoftwareRevision("2026.4.2.0");
    auto device = createWithImplementation<IDevice, CaptureFbParentDevice>(context, nullptr, "device", info);
    auto folder = Folder(context, device, "capture_folder");
    createCaptureWithParent(folder);

    ASSERT_EQ(captureFb.getPropertyValue("DeviceDescription"), String("RT Simulator"));
    ASSERT_EQ(captureFb.getPropertyValue("SerialNumber"), String("DS12345677"));
    ASSERT_EQ(captureFb.getPropertyValue("HardwareVersion"), String("9999.1.2345"));
    ASSERT_EQ(captureFb.getPropertyValue("SoftwareVersion"), String("2026.4.2.0"));
    for (const auto* name : {"DeviceDescription", "SerialNumber", "HardwareVersion", "SoftwareVersion"})
    {
        ASSERT_FALSE(captureFb.getProperty(name).getReadOnly());
        captureFb.setPropertyValue(name, "replacement");
    }

    ASSERT_FALSE(captureFb.getProperty("DeviceId").getReadOnly());
    ASSERT_FALSE(captureFb.getProperty("VendorData").getReadOnly());
    captureFb.setPropertyValue("DeviceId", 42);
    captureFb.setPropertyValue("VendorData", "vendor data");
    ASSERT_EQ(captureFb.getPropertyValue("DeviceId"), Integer(42));
    ASSERT_EQ(captureFb.getPropertyValue("VendorData"), String("vendor data"));
}

TEST_F(CaptureFbTest, MissingDevicePropertiesRemainEditable)
{
    auto info = DeviceInfo("test://device", "Device");
    info.setModel("Reference device");
    info.setSerialNumber("DevSer0");
    auto device = createWithImplementation<IDevice, CaptureFbParentDevice>(context, nullptr, "device", info);
    createCaptureWithParent(device);

    ASSERT_EQ(captureFb.getPropertyValue("DeviceDescription"), String("Reference device"));
    ASSERT_FALSE(captureFb.getProperty("DeviceDescription").getReadOnly());
    ASSERT_FALSE(captureFb.getProperty("SerialNumber").getReadOnly());
    for (const auto* name : {"HardwareVersion", "SoftwareVersion"})
    {
        ASSERT_EQ(captureFb.getPropertyValue(name), String(""));
        ASSERT_FALSE(captureFb.getProperty(name).getReadOnly());
        captureFb.setPropertyValue(name, "manual");
    }
}

TEST_F(CaptureFbTest, InheritRootDeviceProperties)
{
    auto rootInfo = DeviceInfo("test://root", "Root");
    rootInfo.setModel("Root model");
    rootInfo.setSerialNumber("Root serial");
    auto root = createWithImplementation<IDevice, CaptureFbParentDevice>(context, nullptr, "root", rootInfo);
    auto childInfo = DeviceInfo("test://child", "Child");
    childInfo.setModel("Child model");
    childInfo.setSerialNumber("Child serial");
    auto child = createWithImplementation<IDevice, CaptureFbParentDevice>(context, root, "child", childInfo);
    auto folder = Folder(context, child, "capture_folder");
    createCaptureWithParent(folder);

    ASSERT_EQ(captureFb.getPropertyValue("DeviceDescription"), String("Root model"));
    ASSERT_EQ(captureFb.getPropertyValue("SerialNumber"), String("Root serial"));
    ASSERT_EQ(captureFb.getPropertyValue("HardwareVersion"), String(""));
    ASSERT_EQ(captureFb.getPropertyValue("SoftwareVersion"), String(""));
    for (const auto* name : {"DeviceDescription", "SerialNumber", "HardwareVersion", "SoftwareVersion"})
    {
        ASSERT_FALSE(captureFb.getProperty(name).getReadOnly());
        captureFb.setPropertyValue(name, "manual");
        ASSERT_EQ(captureFb.getPropertyValue(name), String("manual"));
    }
}

TEST_F(CaptureFbTest, TestCreateInterface)
{
    ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");

    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 0u);
    createProc();
    createProc();
    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 2u);

    int lstId = captureFb.getFunctionBlocks().getItemAt(1).getPropertyValue("InterfaceId");

    ProcedurePtr removeProc = captureFb.getPropertyValue("RemoveInterface");
    removeProc(0);

    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 1u);
    ASSERT_EQ(captureFb.getFunctionBlocks().getItemAt(0).getPropertyValue("InterfaceId"), lstId);
}

TEST_F(CaptureFbTest, TestCaptureStatusReceived)
{
    EXPECT_CALL(*ethernetWrapper, sendPacket(_)).Times(AtLeast(1));

    uint64_t deviceId = captureFb.getPropertyValue("DeviceId");
    std::string deviceDescription = captureFb.getPropertyValue("DeviceDescription");
    std::string serialNumber = captureFb.getPropertyValue("SerialNumber");
    std::string softwareVersion = captureFb.getPropertyValue("SoftwareVersion");
    std::string hardwareVersion =  captureFb.getPropertyValue("HardwareVersion");
    std::string vendorData = captureFb.getPropertyValue("VendorData");
    size_t vendorDataLen = vendorData.length();

    auto checker = [&]()
    {
        std::scoped_lock lock{packedReceivedSync};

        if (!lastReceivedPacket.isValid())
            return false;

        if (lastReceivedPacket.getPayload().getMessageType() != ASAM::CMP::CmpHeader::MessageType::status)
            return false;

        if (lastReceivedPacket.getDeviceId() != deviceId)
            return false;

        if (static_cast<ASAM::CMP::CaptureModulePayload&>(lastReceivedPacket.getPayload()).getDeviceDescription() != deviceDescription)
            return false;

        if (static_cast<ASAM::CMP::CaptureModulePayload&>(lastReceivedPacket.getPayload()).getSerialNumber() != serialNumber)
            return false;

        if (static_cast<ASAM::CMP::CaptureModulePayload&>(lastReceivedPacket.getPayload()).getHardwareVersion() != hardwareVersion)
            return false;

        if (static_cast<ASAM::CMP::CaptureModulePayload&>(lastReceivedPacket.getPayload()).getSoftwareVersion() != softwareVersion)
            return false;

        if (static_cast<ASAM::CMP::CaptureModulePayload&>(lastReceivedPacket.getPayload()).getVendorDataLength() != vendorDataLen)
            return false;

        const uint8_t* receivedVendorData = static_cast<ASAM::CMP::CaptureModulePayload&>(lastReceivedPacket.getPayload()).getVendorData();

        for (size_t i = 0; i < vendorDataLen; ++i)
        {
            if (vendorData[i] != receivedVendorData[i])
                return false;
        }

        return true;
    };

    size_t timeElapsed = 0;
    auto stTime = std::chrono::steady_clock::now();
    while (!checker() && timeElapsed < 2500)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto curTime = std::chrono::steady_clock::now();
        timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - stTime).count();
    }

    ASSERT_TRUE(checker());

    deviceId = (deviceId == 1 ? 2 : 1);
    deviceDescription = "NewDescription";
    serialNumber = "NewSerialNumber";
    hardwareVersion = "NewHWVEr";
    softwareVersion = "NewSFVer";
    vendorData = "NewVendorDat";
    vendorDataLen = vendorData.length();

    captureFb.setPropertyValue("DeviceId", deviceId);
    captureFb.setPropertyValue("DeviceDescription", deviceDescription);
    captureFb.setPropertyValue("SerialNumber", serialNumber);
    captureFb.setPropertyValue("HardwareVersion", hardwareVersion);
    captureFb.setPropertyValue("SoftwareVersion", softwareVersion);
    captureFb.setPropertyValue("VendorData", vendorData);

    timeElapsed = 0;
    stTime = std::chrono::steady_clock::now();
    while (!checker() && timeElapsed < 2500)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto curTime = std::chrono::steady_clock::now();
        timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - stTime).count();
    }

    ASSERT_TRUE(checker());
}

TEST_F(CaptureFbTest, TestBeginUpdateEndUpdate)
{
    EXPECT_CALL(*ethernetWrapper, sendPacket(_)).Times(AtLeast(0));

    uint32_t oldDeviceId = captureFb.getPropertyValue("DeviceId");
    std::string oldDeviceDescription = captureFb.getPropertyValue("DeviceDescription");
    std::string oldSerialNumber = captureFb.getPropertyValue("SerialNumber");
    std::string oldSoftwareVersion = captureFb.getPropertyValue("SoftwareVersion");
    std::string oldHardwareVersion = captureFb.getPropertyValue("HardwareVersion");
    std::string oldVendorData = captureFb.getPropertyValue("VendorData");

    size_t deviceId = (oldDeviceId == 1 ? 2 : 1);
    std::string deviceDescription = "NewDescription";
    std::string serialNumber = "newSerialNumber";
    std::string hardwareVersion = "NewHWVEr";
    std::string softwareVersion = "NewSFVer";
    std::string vendorData = "NewVendorData";

    ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");
    ProcedurePtr removeProc = captureFb.getPropertyValue("RemoveInterface");

    captureFb.beginUpdate();
    captureFb.setPropertyValue("DeviceId", deviceId);
    captureFb.setPropertyValue("DeviceDescription", deviceDescription);
    captureFb.setPropertyValue("SerialNumber", serialNumber);
    captureFb.setPropertyValue("SoftwareVersion", softwareVersion);
    captureFb.setPropertyValue("HardwareVersion", hardwareVersion);
    captureFb.setPropertyValue("VendorData", vendorData);

    ASSERT_EQ(captureFb.getPropertyValue("DeviceId"), oldDeviceId);
    ASSERT_EQ(captureFb.getPropertyValue("DeviceDescription"), oldDeviceDescription);
    ASSERT_EQ(captureFb.getPropertyValue("SerialNumber"), oldSerialNumber);
    ASSERT_EQ(captureFb.getPropertyValue("SoftwareVersion"), oldSoftwareVersion);
    ASSERT_EQ(captureFb.getPropertyValue("HardwareVersion"), oldHardwareVersion);
    ASSERT_EQ(captureFb.getPropertyValue("VendorData"), oldVendorData);

    ASSERT_ANY_THROW(createProc());
    ASSERT_ANY_THROW(removeProc(0));
    captureFb.endUpdate();

    ASSERT_EQ(captureFb.getPropertyValue("DeviceId"), deviceId);
    ASSERT_EQ(captureFb.getPropertyValue("DeviceDescription"), deviceDescription);
    ASSERT_EQ(captureFb.getPropertyValue("SerialNumber"), serialNumber);
    ASSERT_EQ(captureFb.getPropertyValue("SoftwareVersion"), softwareVersion);
    ASSERT_EQ(captureFb.getPropertyValue("HardwareVersion"), hardwareVersion);
    ASSERT_EQ(captureFb.getPropertyValue("VendorData"), vendorData);

    ASSERT_NO_THROW(createProc());
    ASSERT_NO_THROW(removeProc(0));

}

TEST_F(CaptureFbTest, TestInterfaceIdManager)
{
    EXPECT_CALL(*ethernetWrapper, sendPacket(_)).Times(AtLeast(0));

    ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");
    createProc();

    FunctionBlockPtr intf = captureFb.getFunctionBlocks().getItemAt(0);

    int32_t id = intf.getPropertyValue("InterfaceId");
    int32_t newId = (id == 1 ? 2 : 1);
    intf.setPropertyValue("InterfaceId", newId);

    createProc();
    FunctionBlockPtr intf2 = captureFb.getFunctionBlocks().getItemAt(1);
    int32_t idToCheck = intf2.getPropertyValue("InterfaceId");
    if (idToCheck != id)
        intf2.setPropertyValue("InterfaceId", id);

    idToCheck = intf2.getPropertyValue("InterfaceId");
    ASSERT_EQ(idToCheck, id);
}


TEST_F(CaptureFbTest, TestStatusPacketConsistency)
{
    EXPECT_CALL(*ethernetWrapper, sendPacket(_)).Times(AtLeast(1));

    uint32_t oldDeviceId = captureFb.getPropertyValue("DeviceId");
    std::string oldDeviceDescription = captureFb.getPropertyValue("DeviceDescription");
    std::string oldSerialNumber = captureFb.getPropertyValue("SerialNumber");

    size_t deviceId = (oldDeviceId == 1 ? 2 : 1);
    std::string deviceDescription = "NewDescription";
    std::string serialNumber = "newSerialNumber";

    auto checkPacket = [](ASAM::CMP::Packet& packet, const uint64_t deviceId, const std::string& deviceDescription, const std::string& serialNumber)
    {
        if (packet.getPayload().getMessageType() != ASAM::CMP::CmpHeader::MessageType::status)
            return false;

        if (packet.getDeviceId() != deviceId)
            return false;

        if (static_cast<ASAM::CMP::CaptureModulePayload&>(packet.getPayload()).getDeviceDescription() != deviceDescription)
            return false;

        if (static_cast<ASAM::CMP::CaptureModulePayload&>(packet.getPayload()).getSerialNumber() != serialNumber)
            return false;

        return true;
    };

    bool fstPacketReceived = false, sndPacketReceived = false, otherPacketReceived = false;
    auto checker = [&]() {
        std::scoped_lock lock{packedReceivedSync};

        if (!lastReceivedPacket.isValid())
            return;

        if (!fstPacketReceived)
        {
            if (checkPacket(lastReceivedPacket, oldDeviceId, oldDeviceDescription, oldSerialNumber))
            {
                fstPacketReceived = true;
            }
            else
            {
                otherPacketReceived = true;
            }
        }
        else
        {
            if (checkPacket(lastReceivedPacket, deviceId, deviceDescription, serialNumber))
            {
                sndPacketReceived = true;
            }
            else if (!checkPacket(lastReceivedPacket, oldDeviceId, oldDeviceDescription, oldSerialNumber))
            {
                otherPacketReceived = true;
            }
        }
    };

    auto timeElapsed = 0;
    auto stTime = std::chrono::steady_clock::now();
    while (!fstPacketReceived && timeElapsed < 2500)
    {
        checker();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto curTime = std::chrono::steady_clock::now();
        timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - stTime).count();
    }

    captureFb.beginUpdate();
    captureFb.setPropertyValue("DeviceId", deviceId);
    captureFb.setPropertyValue("DeviceDescription", deviceDescription);
    captureFb.setPropertyValue("SerialNumber", serialNumber);
    captureFb.endUpdate();

    timeElapsed = 0;
    stTime = std::chrono::steady_clock::now();
    while (!sndPacketReceived && timeElapsed < 2500)
    {
        checker();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto curTime = std::chrono::steady_clock::now();
        timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - stTime).count();
    }

    ASSERT_FALSE(otherPacketReceived);
}

TEST_F(CaptureFbTest, TestInterfacesShareSameStreamId)
{
    EXPECT_CALL(*ethernetWrapper, sendPacket(_)).Times(AtLeast(0));

    ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");
    createProc();
    createProc();

    auto itf0 = captureFb.getFunctionBlocks().getItemAt(0), itf1 = captureFb.getFunctionBlocks().getItemAt(1);

    itf0.setPropertyValue("PayloadType", 1);
    itf1.setPropertyValue("PayloadType", 1);

    ProcedurePtr createStreamProc0 = itf0.getPropertyValue("AddStream"), createStreamProc1 = itf1.getPropertyValue("AddStream");
    createStreamProc0();
    createStreamProc0();
    createStreamProc1();

    auto streamFb00 = itf0.getFunctionBlocks().getItemAt(0),
         streamFb01 = itf0.getFunctionBlocks().getItemAt(1),
         streamFb10 = itf1.getFunctionBlocks().getItemAt(0);

    int streamId00 = streamFb00.getPropertyValue("StreamId"),
        streamId01 = streamFb01.getPropertyValue("StreamId"),
        streamId10 = streamFb10.getPropertyValue("StreamId");

    EXPECT_NE(streamId00, streamId01);
    streamFb00.setPropertyValue("StreamId", streamId01);
    streamId00 = streamFb00.getPropertyValue("StreamId");
    EXPECT_NE(streamId00, streamId01);

    streamFb10.setPropertyValue("StreamId", streamId00);
    streamId10 = streamFb10.getPropertyValue("StreamId");
    EXPECT_EQ(streamId00, streamId10);
}
