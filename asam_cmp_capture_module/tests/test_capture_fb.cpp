#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/capture_fb.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <gtest/gtest.h>

#include <asam_cmp_common_lib/ethernet_pcpp_mock.h>
#include <asam_cmp/decoder.h>

using namespace daq;
using namespace testing;

class CaptureFbTest : public testing::Test
{
protected:
    CaptureFbTest()
    : ethernetWrapper(std::make_shared<asam_cmp_common_lib::EthernetPcppMock>())
    {
        auto startStub = []() {};
        auto stopStub = []() {};

        auto sendPacketStub = [](StringPtr deviceName, const std::vector<uint8_t>& data)
        {
                std::cout << "default PacketReceivedCallback is triggered\n";
        };

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
        context = Context(Scheduler(logger), logger, nullptr, nullptr, nullptr);
        const StringPtr captureModuleId = "asam_cmp_capture_fb";
        selectedDevice = "device1";
        modules::asam_cmp_capture_module::CaptureFbInit init = {ethernetWrapper, selectedDevice};
        captureFb =
            createWithImplementation<IFunctionBlock, modules::asam_cmp_capture_module::CaptureFb>(
            context, nullptr, captureModuleId, init);
    }

protected:
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppMock> ethernetWrapper;
    ListPtr<StringPtr> names;
    ListPtr<StringPtr> descriptions;

    StringPtr selectedDevice;
    ContextPtr context;
    FunctionBlockPtr captureFb;

    std::mutex packedReceivedSync;
    ASAM::CMP::Packet lastReceivedPacket;
    ASAM::CMP::Decoder decoder;

    void onPacketSendCb(const std::vector<uint8_t>& data)
    {
        std::scoped_lock lock(packedReceivedSync);
        std::cout << "onPacketSend detected\n";
        lastReceivedPacket = *(decoder.decode(data.data(), data.size()).back().get());
    };
};

TEST_F(CaptureFbTest, CreateCaptureModule)
{
    ASSERT_NE(captureFb, nullptr);
}

TEST_F(CaptureFbTest, JumboFramesNotAllowed)
{
    ASSERT_FALSE(captureFb.getPropertyValue("AllowJumboFrames"));
    EXPECT_ANY_THROW(captureFb.setPropertyValue("AllowJumboFrames", true));
}

TEST_F(CaptureFbTest, TestCreateInterface)
{
    ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");

    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 0);
    createProc();
    createProc();
    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 2);

    int lstId = captureFb.getFunctionBlocks().getItemAt(1).getPropertyValue("InterfaceId");

    ProcedurePtr removeProc = captureFb.getPropertyValue("RemoveInterface");
    removeProc(0);

    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 1);
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
        std::scoped_lock lock(packedReceivedSync);

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

        for (int i = 0; i < vendorDataLen; ++i)
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
        std::scoped_lock lock(packedReceivedSync);

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
