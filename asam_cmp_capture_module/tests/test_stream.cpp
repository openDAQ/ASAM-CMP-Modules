#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/capture_fb.h>
#include <asam_cmp_capture_module/interface_fb.h>
#include <asam_cmp_capture_module/stream_fb.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <gtest/gtest.h>

#include <asam_cmp_common_lib/ethernet_pcpp_mock.h>
#include <asam_cmp/decoder.h>
#include <asam_cmp/interface_payload.h>

using namespace daq;
using namespace testing;

class StreamFbTest: public testing::Test
{
protected:
    StreamFbTest()
        : ethernetWrapper(std::make_shared<asam_cmp_common_lib::EthernetPcppMock>())
    {
        auto startStub = []() {};
        auto stopStub = []() {};

        auto sendPacketStub = [](StringPtr deviceName, const std::vector<uint8_t>& data) {};

        ON_CALL(*ethernetWrapper, startCapture(_, _)).WillByDefault(startStub);
        ON_CALL(*ethernetWrapper, stopCapture(_)).WillByDefault(stopStub);
        ON_CALL(*ethernetWrapper, getEthernetDevicesNamesList()).WillByDefault(Return(names));
        ON_CALL(*ethernetWrapper, getEthernetDevicesDescriptionsList()).WillByDefault(Return(descriptions));
        ON_CALL(*ethernetWrapper, sendPacket(_, _))
            .WillByDefault(WithArgs<0, 1>(
                Invoke([&](StringPtr deviceName, const std::vector<uint8_t>& data) { this->onPacketSendCb(deviceName, data); })));

        auto logger = Logger();
        context = Context(Scheduler(logger), logger, nullptr, nullptr, nullptr);
        const StringPtr captureModuleId = "asam_cmp_capture_fb";
        selectedDevice = "device1";
        modules::asam_cmp_capture_module::CaptureFbInit init = {ethernetWrapper, selectedDevice};
        captureFb =
            createWithImplementation<IFunctionBlock, modules::asam_cmp_capture_module::CaptureFb>(context, nullptr, captureModuleId, init);

        ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");
        createProc();

        interfaceFb = captureFb.getFunctionBlocks().getItemAt(0);
    }

protected:
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppMock> ethernetWrapper;
    ListPtr<StringPtr> names;
    ListPtr<StringPtr> descriptions;

    StringPtr selectedDevice;
    ContextPtr context;
    FunctionBlockPtr captureFb;
    FunctionBlockPtr interfaceFb;

    std::mutex packetReceivedSync;
    ASAM::CMP::Packet lastReceivedPacket;
    ASAM::CMP::Decoder decoder;

    void onPacketSendCb(StringPtr deviceName, const std::vector<uint8_t>& data)
    {
        std::scoped_lock lock(packetReceivedSync);
        std::cout << "onPacketSend detected\n";
        lastReceivedPacket = *(decoder.decode(data.data(), data.size()).back().get());
    };
};

TEST_F(StreamFbTest, CreateStream)
{
    ProcedurePtr createProc = interfaceFb.getPropertyValue("AddStream");
    interfaceFb.setPropertyValue("PayloadType", 1);
    createProc();
    createProc();

    auto s1 = interfaceFb.getFunctionBlocks().getItemAt(0), s2 = interfaceFb.getFunctionBlocks().getItemAt(1);

    ASSERT_NE(s1.getPropertyValue("StreamId"), s2.getPropertyValue("StreamId"));
}

TEST_F(StreamFbTest, TestInterfaceStatusWithStreamsReceived)
{
    EXPECT_CALL(*ethernetWrapper, sendPacket(_, _)).Times(AtLeast(1));

    int streamsCnt = 0;
    std::vector<int> streams = {};
    std::string vendorData = captureFb.getPropertyValue("VendorData");
    size_t vendorDataLen = vendorData.length();

    auto checker = [&]()
    {
        std::scoped_lock lock(packetReceivedSync);

         if (!lastReceivedPacket.isValid())
            return false;

        if (lastReceivedPacket.getPayload().getMessageType() != ASAM::CMP::CmpHeader::MessageType::status)
            return false;

        if (lastReceivedPacket.getPayload().getType() != ASAM::CMP::PayloadType::ifStatMsg)
            return false;

        if (static_cast<ASAM::CMP::InterfacePayload&>(lastReceivedPacket.getPayload()).getStreamIdsCount() != streamsCnt)
            return false;

        for (int i = 0; i < streamsCnt; ++i)
        {
            if (static_cast<ASAM::CMP::InterfacePayload&>(lastReceivedPacket.getPayload()).getStreamIds()[i] != streams[i])
                return false;
        }

        const uint8_t* receivedVendorData = static_cast<ASAM::CMP::InterfacePayload&>(lastReceivedPacket.getPayload()).getVendorData();

        for (int i = 0; i < vendorDataLen; ++i)
        {
            if (vendorData[i] != receivedVendorData[i])
                return false;
        }

        return true;
    };

    auto catchInterfaceStatus = [&](){
        size_t timeElapsed = 0;
        auto stTime = std::chrono::steady_clock::now();
        bool hasCorrectInterfaceStatus = false;
        while (!hasCorrectInterfaceStatus && timeElapsed < 250000000000)
        {
            hasCorrectInterfaceStatus |= checker();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            auto curTime = std::chrono::steady_clock::now();
            timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - stTime).count();
        }

        ASSERT_TRUE(hasCorrectInterfaceStatus);
    };

    catchInterfaceStatus();

    ProcedurePtr createProc = interfaceFb.getPropertyValue("AddStream");
    interfaceFb.setPropertyValue("PayloadType", 1);
    createProc();
    createProc();

    auto s1 = interfaceFb.getFunctionBlocks().getItemAt(0), s2 = interfaceFb.getFunctionBlocks().getItemAt(1);

    streamsCnt = 2;
    streams = {s1.getPropertyValue("StreamId"), s2.getPropertyValue("StreamId")};

    catchInterfaceStatus();

    ProcedurePtr removeProc = interfaceFb.getPropertyValue("RemoveStream");
    removeProc(0);
    streamsCnt = 1;
    s1 = interfaceFb.getFunctionBlocks().getItemAt(0);
    streams = {s1.getPropertyValue("StreamId")};

    catchInterfaceStatus();
}
