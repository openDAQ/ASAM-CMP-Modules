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
#include "include/ref_can_channel_impl.h"
#include <asam_cmp/can_payload.h>
#include <asam_cmp/can_fd_payload.h>

using namespace daq;
using namespace testing;


class TimeStub
{
public:
    TimeStub()
    {
        startTime = std::chrono::steady_clock::now();
        auto startAbsTime = std::chrono::system_clock::now();
        microSecondsFromEpochToDeviceStart = std::chrono::duration_cast<std::chrono::microseconds>(startAbsTime.time_since_epoch());
    }

    std::chrono::microseconds getMicroSecondsSinceDeviceStart() const
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto microSecondsSinceDeviceStart = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
        return microSecondsSinceDeviceStart;
    }

    std::chrono::microseconds getMicroSecondsFromEpochToDeviceStart() const
    {
        return microSecondsFromEpochToDeviceStart;
    }

private:
    std::chrono::steady_clock::time_point startTime;
    std::chrono::microseconds microSecondsFromEpochToDeviceStart;
};

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

    void triggerCanChannel(size_t samplesCount)
    {
        auto curTime = timeStub.getMicroSecondsSinceDeviceStart();
        auto chPrivate = canChannel.asPtr<IRefChannel>();
        chPrivate->collectSamples(curTime, samplesCount);
    }

    void onPacketSendCb(StringPtr deviceName, const std::vector<uint8_t>& data)
    {
        std::scoped_lock lock(packedReceivedSync);
        std::cout << "onPacketSend detected\n";
        for (const auto& e : decoder.decode(data.data(), data.size()))
            receivedPackets.push(e);
    };

    void rawCanFdFrameCapture(const CANData& data)
    {
        capturedFrames.emplace_back();
        CANData& frame = capturedFrames.back();

        frame.arbId = data.arbId;
        frame.length = data.length;
        memcpy(frame.data, data.data, frame.length);
    }


    void rawCanFrameCapture(const CANData& data)
    {
        if (data.length <= 8)
        {
            capturedFrames.emplace_back();
            CANData& frame = capturedFrames.back();

            frame.arbId = data.arbId;
            frame.length = data.length;
            memcpy(frame.data, data.data, frame.length);
        }
    }

protected:
    TimeStub timeStub;
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppMock> ethernetWrapper;
    ListPtr<StringPtr> names;
    ListPtr<StringPtr> descriptions;

    StringPtr selectedDevice;
    ContextPtr context;
    FunctionBlockPtr captureFb;
    FunctionBlockPtr interfaceFb;
    ChannelPtr canChannel;
    std::vector<CANData> capturedFrames;

    std::mutex packedReceivedSync;
    std::queue<std::shared_ptr<ASAM::CMP::Packet>> receivedPackets;
    ASAM::CMP::Decoder decoder;
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

TEST_F(StreamFbTest, TestPacketsAreSent)
{
    auto rawFramesCapture = [&](const CANData& data) {
        rawCanFrameCapture(data);
    };
    RefCANChannelInit initCanCh{
        timeStub.getMicroSecondsSinceDeviceStart(), timeStub.getMicroSecondsFromEpochToDeviceStart(), rawFramesCapture};
    canChannel = createWithImplementation<IChannel, RefCANChannelImpl>(this->context, nullptr, "refcanch", initCanCh);

    EXPECT_CALL(*ethernetWrapper, sendPacket(_, _)).Times(AtLeast(0));

    ProcedurePtr createProc = interfaceFb.getPropertyValue("AddStream");
    interfaceFb.setPropertyValue("PayloadType", 1);
    createProc();
    auto streamFb = interfaceFb.getFunctionBlocks().getItemAt(0);

    uint8_t streamId = streamFb.getPropertyValue("StreamId");
    uint32_t interfaceId = interfaceFb.getPropertyValue("InterfaceId");
    uint16_t deviceId = captureFb.getPropertyValue("DeviceId");

    SignalPtr sender = canChannel.getSignals().getItemAt(0);

    streamFb.getInputPorts().getItemAt(0).connect(sender);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    int framesToSend{5};
    triggerCanChannel(framesToSend);


    int receivedCanFrames = 0;
    auto checker = [&]() -> bool
    {
        std::scoped_lock lock(packedReceivedSync);
        if (receivedPackets.empty())
            return false;

        auto packet = *(receivedPackets.front());
        receivedPackets.pop();

        if (!packet.isValid())
            return false;

        if (packet.getPayload().getMessageType() != ASAM::CMP::CmpHeader::MessageType::data)
            return false;

        if (packet.getDeviceId() != deviceId)
            return false;

        if (packet.getStreamId() != streamId)
            return false;

        if (packet.getPayload().getRawPayloadType() != uint8_t(ASAM::CMP::PayloadType::can))
            return false;

        if (packet.getInterfaceId() != interfaceId)
            return false;

        CANData& referenceCanFrame = capturedFrames[receivedCanFrames];

        if (static_cast<ASAM::CMP::CanPayload&>(packet.getPayload()).getId() != referenceCanFrame.arbId)
            return false;

        if (static_cast<ASAM::CMP::CanPayload&>(packet.getPayload()).getDataLength() != referenceCanFrame.length)
            return false;

        auto rawData = static_cast<ASAM::CMP::CanPayload&>(packet.getPayload()).getData();
        for (int i = 0; i < referenceCanFrame.length; ++i)
        {
            if (rawData[i] != referenceCanFrame.data[i])
                return false;
        }

        ++receivedCanFrames;
        return receivedCanFrames == framesToSend;
    };

    size_t timeElapsed = 0;
    auto stTime = std::chrono::steady_clock::now();
    while (!checker() && timeElapsed < 2500)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto curTime = std::chrono::steady_clock::now();
        timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - stTime).count();
    }

    ASSERT_EQ(receivedCanFrames, framesToSend);
}
