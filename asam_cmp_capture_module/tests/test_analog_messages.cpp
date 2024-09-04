
#include <asam_cmp/analog_payload.h>
#include <asam_cmp/decoder.h>

#include <gtest/gtest.h>
#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/capture_fb.h>
#include <asam_cmp_capture_module/input_descriptors_validator.h>

#include <asam_cmp_common_lib/ethernet_pcpp_mock.h>

#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/logger_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>

#include "include/ref_channel_impl.h"
#include "include/time_stub.h"

using namespace daq;
using namespace testing;

class AnalogMessagesTest : public testing::Test
{
protected:
    using SourceType = typename SampleTypeToType<SampleType::Int16>::Type;

    AnalogMessagesTest()
        : ethernetWrapper(std::make_shared<asam_cmp_common_lib::EthernetPcppMock>())
    {
        auto startStub = []() {};
        auto stopStub = []() {};

        auto sendPacketStub = [](StringPtr deviceName, const std::vector<uint8_t>& data) {};

        ON_CALL(*ethernetWrapper, startCapture(_)).WillByDefault(startStub);
        ON_CALL(*ethernetWrapper, stopCapture()).WillByDefault(stopStub);
        ON_CALL(*ethernetWrapper, getEthernetDevicesNamesList()).WillByDefault(Return(names));
        ON_CALL(*ethernetWrapper, getEthernetDevicesDescriptionsList()).WillByDefault(Return(descriptions));
        ON_CALL(*ethernetWrapper, sendPacket(_))
            .WillByDefault(WithArgs<0>(Invoke([&](const std::vector<uint8_t>& data) { this->onPacketSendCb(data); })));

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

    void onPacketSendCb(const std::vector<uint8_t>& data)
    {
        std::scoped_lock lock(packedReceivedSync);
        std::cout << "onPacketSend detected\n";
        for (const auto& e : decoder.decode(data.data(), data.size()))
            receivedPackets.push(e);
    };

    int analogCallback()
    {
        if (analogValue == 11)
            analogValue = -10;

        sentAnalogSamples.emplace_back(analogValue);
        return analogValue++;
    }

    template <SampleType SrcType>
    void testAnalogPackets(bool setScale);

    template <typename T>
    bool checkSamples(const ASAM::CMP::AnalogPayload& analogPayload, int& receivedSamples);

protected:
    TimeStub timeStub;
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppMock> ethernetWrapper;
    ListPtr<StringPtr> names;
    ListPtr<StringPtr> descriptions;
    StringPtr selectedDevice;

    ContextPtr context;
    FunctionBlockPtr captureFb;
    FunctionBlockPtr interfaceFb;

    std::mutex packedReceivedSync;
    std::queue<std::shared_ptr<ASAM::CMP::Packet>> receivedPackets;
    ASAM::CMP::Decoder decoder;
    std::vector<double> sentAnalogSamples;
    double analogValue{0};
};

template <typename T>
bool AnalogMessagesTest::checkSamples(const ASAM::CMP::AnalogPayload& analogPayload, int& receivedSamples)
{
    auto rawData = reinterpret_cast<const T*>(analogPayload.getData());
    auto receivedSamplesCnt = analogPayload.getSamplesCount();

    for (int i = 0; i < receivedSamplesCnt; ++i, rawData++)
    {
        double calculatedValue = (*rawData) * analogPayload.getSampleScalar() + analogPayload.getSampleOffset();
        if (abs(sentAnalogSamples[receivedSamples++] - calculatedValue) > analogPayload.getSampleScalar())
        {
            std::cout << "Wrong: " << sentAnalogSamples[receivedSamples - 1] << ", " << calculatedValue << std::endl;
            return false;
        }
    }
    return true;
}

template <SampleType SrcType>
void AnalogMessagesTest::testAnalogPackets(bool setScale)
{
    EXPECT_CALL(*ethernetWrapper, sendPacket(_)).Times(AtLeast(0));

    ProcedurePtr createProc = interfaceFb.getPropertyValue("AddStream");
    interfaceFb.setPropertyValue("PayloadType", 3);
    createProc();
    auto streamFb = interfaceFb.getFunctionBlocks().getItemAt(0);

    uint8_t streamId = streamFb.getPropertyValue("StreamId");
    uint32_t interfaceId = interfaceFb.getPropertyValue("InterfaceId");
    uint16_t deviceId = captureFb.getPropertyValue("DeviceId");

    daq::InputChannelStubInit init{
        0, 200, timeStub.getMicroSecondsSinceDeviceStart(), timeStub.getMicroSecondsFromEpochToDeviceStart(), [&]() {
            return analogCallback();
        }};
    ChannelPtr analogChannel = createWithImplementation<IChannel, InputChannelStubImpl>(this->context, nullptr, "refch", init);
    analogChannel.asPtr<IInputChannelStub>()->initDescriptors();
    analogChannel.setPropertyValue("SampleType", static_cast<int>(SrcType));
    if (setScale)
    {
        analogChannel.setPropertyValue("ClientSideScaling", true);
    }

    SignalPtr analogSignal = analogChannel.getSignals()[0];

    streamFb.getInputPorts().getItemAt(0).connect(analogSignal);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto curTime = timeStub.getMicroSecondsSinceDeviceStart();
    auto chPrivate = analogChannel.asPtr<IInputChannelStub>();
    chPrivate->collectSamples(curTime);

    int receivedSamples = 0;
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

        if (packet.getPayload().getType() != ASAM::CMP::PayloadType::analog)
            return false;

        if (packet.getInterfaceId() != interfaceId)
            return false;

        const auto& analogPayload = static_cast<ASAM::CMP::AnalogPayload&>(packet.getPayload());
        if (analogPayload.getSampleDt() == ASAM::CMP::AnalogPayload::SampleDt::aInt16)
        {
            if (!checkSamples<int16_t>(analogPayload, receivedSamples))
                return false;
        }
        else
        {
            if (!checkSamples<int32_t>(analogPayload, receivedSamples))
                return false;
        }

        return receivedSamples == sentAnalogSamples.size();
    };

    size_t timeElapsed = 0;
    auto stTime = std::chrono::steady_clock::now();
    while (!checker() && timeElapsed < 2500'000'000)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto curTime = std::chrono::steady_clock::now();
        timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - stTime).count();
    }

    ASSERT_EQ(receivedSamples, sentAnalogSamples.size());
}

TEST_F(AnalogMessagesTest, TestAnalogPacketInt16)
{
    testAnalogPackets<SampleType::Int16>(false);
}

TEST_F(AnalogMessagesTest, TestAnalogPacketInt32)
{
    testAnalogPackets<SampleType::Int32>(false);
}

TEST_F(AnalogMessagesTest, TestAnalogPacketFloat32)
{
    testAnalogPackets<SampleType::Float32>(false);
}

TEST_F(AnalogMessagesTest, TestAnalogPacketFloat64)
{
    testAnalogPackets<SampleType::Float64>(false);
}

TEST_F(AnalogMessagesTest, TestAnalogPacketScaled)
{
    testAnalogPackets<SampleType::Float64>(true);
}

