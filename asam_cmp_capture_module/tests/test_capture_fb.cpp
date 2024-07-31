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

        ON_CALL(*ethernetWrapper, startCapture(_, _)).WillByDefault(startStub);
        ON_CALL(*ethernetWrapper, stopCapture(_)).WillByDefault(stopStub);
        ON_CALL(*ethernetWrapper, getEthernetDevicesNamesList()).WillByDefault(Return(names));
        ON_CALL(*ethernetWrapper, getEthernetDevicesDescriptionsList()).WillByDefault(Return(descriptions));
        ON_CALL(*ethernetWrapper, sendPacket(_, _))
            .WillByDefault(WithArgs<0, 1>(Invoke(
                [&](StringPtr deviceName, const std::vector<uint8_t>& data){
            this->onPacketSendCb(deviceName, data);}
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

    void onPacketSendCb(StringPtr deviceName, const std::vector<uint8_t>& data)
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
    EXPECT_CALL(*ethernetWrapper, sendPacket(_, _)).Times(AtLeast(1));

    std::string deviceDescription = captureFb.getPropertyValue("DeviceDescription");
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

        if (static_cast<ASAM::CMP::CaptureModulePayload&>(lastReceivedPacket.getPayload()).getDeviceDescription() != deviceDescription)
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
    while (!checker() && timeElapsed < 250000000000)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto curTime = std::chrono::steady_clock::now();
        timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - stTime).count();
    }

    ASSERT_TRUE(checker());

    deviceDescription = "NewDescription";
    hardwareVersion = "NewHWVEr";
    softwareVersion = "NewSFVer";
    vendorData = "NewVendorDat";
    vendorDataLen = vendorData.length();

    captureFb.setPropertyValue("DeviceDescription", deviceDescription);
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


