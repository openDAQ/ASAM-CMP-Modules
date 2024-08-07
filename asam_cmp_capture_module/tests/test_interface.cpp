#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/capture_fb.h>
#include <asam_cmp_capture_module/interface_fb.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <gtest/gtest.h>

#include <asam_cmp_common_lib/ethernet_pcpp_mock.h>
#include <asam_cmp/decoder.h>
#include <asam_cmp/interface_payload.h>

using namespace daq;
using namespace testing;

class InterfaceFbTest: public testing::Test
{
    protected:
    InterfaceFbTest() : ethernetWrapper(std::make_shared<asam_cmp_common_lib::EthernetPcppMock>())
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
        context = Context(Scheduler(logger), logger, TypeManager(), nullptr, nullptr);
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

TEST_F(InterfaceFbTest, CreateInterface)
{
    ASSERT_NE(interfaceFb, nullptr);
}

TEST_F(InterfaceFbTest, CaptureModuleProperties)
{
    ASSERT_TRUE(interfaceFb.hasProperty("InterfaceId"));
    ASSERT_TRUE(interfaceFb.hasProperty("PayloadType"));
    ASSERT_TRUE(interfaceFb.hasProperty("AddStream"));
    ASSERT_TRUE(interfaceFb.hasProperty("RemoveStream"));
}

TEST_F(InterfaceFbTest, TestSetId)
{
    ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");
    createProc();

    FunctionBlockPtr itf1 = captureFb.getFunctionBlocks().getItemAt(0), itf2 = captureFb.getFunctionBlocks().getItemAt(1);

    uint32_t id1 = itf1.getPropertyValue("InterfaceId"), id2 = itf2.getPropertyValue("InterfaceId");
    ASSERT_NE(id1, id2);

    itf2.setPropertyValue("InterfaceId", id1);
    ASSERT_EQ(itf2.getPropertyValue("InterfaceId"), id2);

    itf2.setPropertyValue("InterfaceId", static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 1);
    ASSERT_EQ(itf2.getPropertyValue("InterfaceId"), id2);

    itf2.setPropertyValue("InterfaceId", id2 + 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_EQ(itf2.getPropertyValue("InterfaceId"), id2 + 1);

    itf2.setPropertyValue("InterfaceId", id1);
    ASSERT_EQ(itf2.getPropertyValue("InterfaceId"), id2 + 1);

    itf2.setPropertyValue("InterfaceId", static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 1);
    ASSERT_EQ(itf2.getPropertyValue("InterfaceId"), id2 + 1);
}


TEST_F(InterfaceFbTest, TestBeginUpdateEndUpdate)
{
    EXPECT_CALL(*ethernetWrapper, sendPacket(_, _)).Times(AtLeast(0));

    size_t oldInterfaceId = interfaceFb.getPropertyValue("InterfaceId");
    size_t oldPayloadType = interfaceFb.getPropertyValue("PayloadType");
    std::string oldVendorData = interfaceFb.getPropertyValue("VendorData");

    size_t interfaceId = (oldInterfaceId == 1 ? 2 : 1);
    size_t payloadType = (oldPayloadType == 0 ? 1 : 0);
    std::string vendorData = "NewVendorData";

    ProcedurePtr createProc = interfaceFb.getPropertyValue("AddStream");
    ProcedurePtr removeProc = interfaceFb.getPropertyValue("RemoveStream");

    interfaceFb.beginUpdate();
    interfaceFb.setPropertyValue("InterfaceId", interfaceId);
    interfaceFb.setPropertyValue("PayloadType", payloadType);
    interfaceFb.setPropertyValue("VendorData", vendorData);

    ASSERT_EQ(interfaceFb.getPropertyValue("InterfaceId"), oldInterfaceId);
    ASSERT_EQ(interfaceFb.getPropertyValue("PayloadType"), oldPayloadType);
    ASSERT_EQ(interfaceFb.getPropertyValue("VendorData"), oldVendorData);
    ASSERT_ANY_THROW(createProc());
    ASSERT_ANY_THROW(removeProc(0));
    interfaceFb.endUpdate();

    ASSERT_EQ(interfaceFb.getPropertyValue("InterfaceId"), interfaceId);
    ASSERT_EQ(interfaceFb.getPropertyValue("PayloadType"), payloadType);
    ASSERT_EQ(interfaceFb.getPropertyValue("VendorData"), vendorData);
    ASSERT_NO_THROW(createProc());
    ASSERT_NO_THROW(removeProc(0));
}

TEST_F(InterfaceFbTest, TestBeginUpdateEndUpdateWithWrongId)
{
    EXPECT_CALL(*ethernetWrapper, sendPacket(_, _)).Times(AtLeast(0));

    ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");
    createProc();
    FunctionBlockPtr itf2 = captureFb.getFunctionBlocks().getItemAt(1);
    size_t deprecatedId = itf2.getPropertyValue("InterfaceId");

    size_t oldInterfaceId = interfaceFb.getPropertyValue("InterfaceId");
    size_t oldPayloadType = interfaceFb.getPropertyValue("PayloadType");

    size_t interfaceId = deprecatedId;
    size_t payloadType = (oldPayloadType == 0 ? 1 : 0);

    interfaceFb.beginUpdate();
    interfaceFb.setPropertyValue("InterfaceId", interfaceId);
    interfaceFb.setPropertyValue("PayloadType", payloadType);

    ASSERT_EQ(interfaceFb.getPropertyValue("InterfaceId"), oldInterfaceId);
    ASSERT_EQ(interfaceFb.getPropertyValue("PayloadType"), oldPayloadType);
    interfaceFb.endUpdate();

    ASSERT_EQ(interfaceFb.getPropertyValue("InterfaceId"), oldInterfaceId);
    ASSERT_EQ(interfaceFb.getPropertyValue("PayloadType"), payloadType);
}

TEST_F(InterfaceFbTest, TestStreamManager)
{
    EXPECT_CALL(*ethernetWrapper, sendPacket(_, _)).Times(AtLeast(0));

    ProcedurePtr createProc = interfaceFb.getPropertyValue("AddStream");
    createProc();
    createProc();

    FunctionBlockPtr stream1 = interfaceFb.getFunctionBlocks().getItemAt(0);
    size_t id1 = stream1.getPropertyValue("StreamId");

    FunctionBlockPtr stream2 = interfaceFb.getFunctionBlocks().getItemAt(1);
    size_t id2 = stream2.getPropertyValue("StreamId");

    stream2.setPropertyValue("StreamId", id1);
    ASSERT_EQ(stream2.getPropertyValue("StreamId"), id2);

    size_t id3 = 47;
    stream1.setPropertyValue("StreamId", id3);
    stream2.setPropertyValue("StreamId", id1);
    ASSERT_EQ(stream2.getPropertyValue("StreamId"), id1);

    size_t newId = id2;
    id2 = id1;
    id1 = id3;

    createProc();
    FunctionBlockPtr stream3 = interfaceFb.getFunctionBlocks().getItemAt(2);
    if (size_t tmp = stream3.getPropertyValue("StreamId"); tmp != newId)
        stream3.setPropertyValue("StreamId", newId);

    ASSERT_EQ(stream3.getPropertyValue("StreamId"), newId);
}

TEST_F(InterfaceFbTest, TestInterfaceStatusWithStreamsReceived)
{
    EXPECT_CALL(*ethernetWrapper, sendPacket(_, _)).Times(AtLeast(1));

    size_t interfaceId = interfaceFb.getPropertyValue("InterfaceId");
    size_t payloadType = interfaceFb.getPropertyValue("PayloadType");
    int streamsCnt = 0;
    std::vector<int> streams = {};
    std::string vendorData = interfaceFb.getPropertyValue("VendorData");
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

        if (static_cast<ASAM::CMP::InterfacePayload&>(lastReceivedPacket.getPayload()).getInterfaceId() !=
            interfaceId)
            return false;

        if (static_cast<ASAM::CMP::InterfacePayload&>(lastReceivedPacket.getPayload()).getInterfaceType() != payloadType)
            return false;

        if (static_cast<ASAM::CMP::InterfacePayload&>(lastReceivedPacket.getPayload()).getInterfaceStatus() !=
            ASAM::CMP::InterfacePayload::InterfaceStatus::linkStatusUp)
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

    auto catchInterfaceStatus = [&]()
    {
        size_t timeElapsed = 0;
        auto stTime = std::chrono::steady_clock::now();
        bool hasCorrectInterfaceStatus = false;
        while (!hasCorrectInterfaceStatus && timeElapsed < 2500)
        {
            hasCorrectInterfaceStatus |= checker();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            auto curTime = std::chrono::steady_clock::now();
            timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - stTime).count();
        }

        ASSERT_TRUE(hasCorrectInterfaceStatus);
    };

    catchInterfaceStatus();


    interfaceFb.setPropertyValue("PayloadType", 1);
    payloadType = interfaceFb.getPropertyValue("PayloadType");

    interfaceFb.setPropertyValue("InterfaceId", (interfaceId == 1 ? 2 : 1));
    interfaceId = interfaceFb.getPropertyValue("InterfaceId");

    ProcedurePtr createProc = interfaceFb.getPropertyValue("AddStream");
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

TEST_F(InterfaceFbTest, TestStatusPacketConsistency)
{
    EXPECT_CALL(*ethernetWrapper, sendPacket(_, _)).Times(AtLeast(1));

    size_t oldInterfaceId = interfaceFb.getPropertyValue("InterfaceId");
    size_t oldPayloadType = interfaceFb.getPropertyValue("PayloadType");

    size_t interfaceId = (oldInterfaceId == 1 ? 2 : 1);
    size_t payloadType = (oldPayloadType == 0 ? 1 : 0);

    auto checkPacket =
        [](ASAM::CMP::Packet& packet, const uint64_t interfaceId, const size_t payloadType)
    {
        if (static_cast<ASAM::CMP::InterfacePayload&>(packet.getPayload()).getInterfaceId() != interfaceId)
            return false;

        if (static_cast<ASAM::CMP::InterfacePayload&>(packet.getPayload()).getInterfaceType() != payloadType)
            return false;

        return true;
    };

    bool fstPacketReceived = false, sndPacketReceived = false, otherPacketReceived = false;
    auto checker = [&]()
    {
        std::scoped_lock lock(packetReceivedSync);

        if (!lastReceivedPacket.isValid())
            return;

        if (lastReceivedPacket.getPayload().getMessageType() != ASAM::CMP::CmpHeader::MessageType::status)
            return;

        if (lastReceivedPacket.getPayload().getType() != ASAM::CMP::PayloadType::ifStatMsg)
            return;

        if (!fstPacketReceived)
        {
            if (checkPacket(lastReceivedPacket, oldInterfaceId, oldPayloadType))
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
            if (checkPacket(lastReceivedPacket, interfaceId, payloadType))
            {
                sndPacketReceived = true;
            }
            else if (!checkPacket(lastReceivedPacket, oldInterfaceId, oldPayloadType))
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

    interfaceFb.beginUpdate();
    interfaceFb.setPropertyValue("InterfaceId", interfaceId);
    interfaceFb.setPropertyValue("PayloadType", payloadType);
    interfaceFb.endUpdate();

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
