#include <asam_cmp_data_sink/calls_multi_map.h>
#include <asam_cmp_data_sink/capture_fb.h>
#include <asam_cmp_data_sink/common.h>
#include <asam_cmp_data_sink/data_handler.h>

#include <asam_cmp/can_payload.h>
#include <asam_cmp/packet.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/reader_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>

using namespace std::chrono_literals;

using namespace daq;
using ASAM::CMP::CanPayload;
using ASAM::CMP::Packet;
using daq::modules::asam_cmp_data_sink_module::CallsMultiMap;
using daq::modules::asam_cmp_data_sink_module::IDataHandler;

size_t waitForSamples(const GenericReaderPtr<IReader>& reader, std::chrono::milliseconds timeout = 100ms)
{
    auto startTime = std::chrono::steady_clock::now();
    auto curTime = startTime;
    while (reader.getAvailableCount() == 0 && curTime - startTime < timeout)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        curTime = std::chrono::steady_clock::now();
    }

    return reader.getAvailableCount();
}

StreamReaderPtr StreamReaderSkipEvents(SignalPtr signal,
                                       SampleType valueType = SampleType::Float64,
                                       SampleType domainType = SampleType::Int64)
{
    return StreamReaderBuilder().setSignal(signal).setValueReadType(valueType).setDomainReadType(domainType).setSkipEvents(true).build();
}

class StreamFbTest : public testing::Test
{
protected:
#pragma pack(push, 1)
    struct CANData
    {
        uint32_t arbId;
        uint8_t length;
        uint8_t data[64];
    };
#pragma pack(pop)

protected:
    void SetUp()
    {
        auto logger = Logger();
        captureFb = createWithImplementation<IFunctionBlock, modules::asam_cmp_data_sink_module::CaptureFb>(
            Context(Scheduler(logger), logger, TypeManager(), nullptr), nullptr, "capture_module_0", callsMultiMap);

        captureFb.getPropertyValue("AddInterface").execute();
        interfaceFb = captureFb.getFunctionBlocks().getItemAt(0);
        interfaceFb.getPropertyValue("AddStream").execute();
        funcBlock = interfaceFb.getFunctionBlocks().getItemAt(0);

        captureFb.setPropertyValue("DeviceId", deviceId);
        interfaceFb.setPropertyValue("InterfaceId", interfaceId);
        funcBlock.setPropertyValue("StreamId", streamId);

        createCanPacket();
    }

    void createCanPacket()
    {
        CanPayload canPayload;
        canPayload.setData(reinterpret_cast<const uint8_t*>(&canData), sizeof(canData));
        canPayload.setId(arbId);

        canPacket = std::make_shared<Packet>();
        canPacket->setPayload(canPayload);
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        canPacket->setTimestamp(timestamp);
        canPacket->setDeviceId(deviceId);
        canPacket->setInterfaceId(interfaceId);
        canPacket->setStreamId(streamId);
    }

protected:
    static constexpr uint16_t deviceId = 0;
    static constexpr uint32_t interfaceId = 1;
    static constexpr uint8_t streamId = 2;

    static constexpr uint32_t arbId = 45;

    static constexpr int canPayloadType = 1;
    static constexpr int analogPayloadType = 3;

protected:
    CallsMultiMap callsMultiMap;
    FunctionBlockPtr captureFb;
    FunctionBlockPtr interfaceFb;
    FunctionBlockPtr funcBlock;

    std::shared_ptr<Packet> canPacket;
    long long timestamp;
    const uint32_t canData = 33;
};

TEST_F(StreamFbTest, NotNull)
{
    ASSERT_NE(funcBlock, nullptr);
}

TEST_F(StreamFbTest, FunctionBlockType)
{
    auto type = funcBlock.getFunctionBlockType();
    ASSERT_EQ(type.getId(), "asam_cmp_stream");
    ASSERT_EQ(type.getName(), "AsamCmpStream");
    ASSERT_EQ(type.getDescription(), "Asam CMP Stream");
}

TEST_F(StreamFbTest, StreamIdProperty)
{
    const int streamId = funcBlock.getPropertyValue("StreamId");
    const int newStreamId = streamId + 1;
    funcBlock.setPropertyValue("StreamId", newStreamId);
    ASSERT_EQ(funcBlock.getPropertyValue("StreamId"), newStreamId);
}

TEST_F(StreamFbTest, SignalsCount)
{
    const auto outputSignals = funcBlock.getSignalsRecursive();
    ASSERT_EQ(outputSignals.getCount(), 1);
}

TEST_F(StreamFbTest, SignalName)
{
    const StringPtr signalName = "Data";

    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    ASSERT_EQ(outputSignal.getName(), signalName);
}

TEST_F(StreamFbTest, CanSignalDescriptor)
{
    const StringPtr name = "CAN";
    constexpr SampleType sampleType = SampleType::Struct;

    interfaceFb.setPropertyValue("PayloadType", canPayloadType);

    const auto descriptor = funcBlock.getSignalsRecursive()[0].getDescriptor();
    ASSERT_EQ(descriptor.getName(), name);
    ASSERT_EQ(descriptor.getSampleType(), sampleType);
    ASSERT_EQ(descriptor.getSampleSize(), sizeof(CANData));
}

TEST_F(StreamFbTest, ReceivePacketWithWrongPayloadType)
{
    interfaceFb.setPropertyValue("PayloadType", analogPayloadType);
    const auto dataHandler = funcBlock.as<IDataHandler>(true);

    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    const StreamReaderPtr reader = StreamReader(outputSignal);

    dataHandler->processData(canPacket);
    const bool haveSamples = waitForSamples(reader);
    ASSERT_FALSE(haveSamples);
}

TEST_F(StreamFbTest, ReadOutputCanSignal)
{
    interfaceFb.setPropertyValue("PayloadType", canPayloadType);
    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    const StreamReaderPtr reader = StreamReaderSkipEvents(outputSignal, SampleType::Struct, SampleType::UInt64);

    callsMultiMap.processPacket(canPacket);
    const auto samplesCount = waitForSamples(reader);
    ASSERT_EQ(samplesCount, 1);

    CANData sample;
    uint64_t domainSample;
    size_t count = 1;
    reader.readWithDomain(&sample, &domainSample, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(domainSample, canPacket->getTimestamp());
    ASSERT_EQ(sample.arbId, arbId);
    ASSERT_EQ(sample.length, sizeof(canData));
    uint32_t checkData = *reinterpret_cast<uint32_t*>(sample.data);
    ASSERT_EQ(checkData, canData);
}

TEST_F(StreamFbTest, ChangeStreamId)
{
    constexpr Int newStreamId = 100;
    interfaceFb.setPropertyValue("PayloadType", canPayloadType);
    funcBlock.setPropertyValue("StreamId", newStreamId);

    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    const StreamReaderPtr reader = StreamReaderSkipEvents(outputSignal, SampleType::Struct, SampleType::Int64);

    callsMultiMap.processPacket(canPacket);
    auto samplesCount = waitForSamples(reader);
    ASSERT_EQ(samplesCount, 0);

    canPacket->setStreamId(newStreamId);
    callsMultiMap.processPacket(canPacket);
    samplesCount = waitForSamples(reader);
    ASSERT_EQ(samplesCount, 1);
}

TEST_F(StreamFbTest, ChangeInterfaceId)
{
    constexpr Int newInterfaceId = 100;
    interfaceFb.setPropertyValue("InterfaceId", newInterfaceId);
    interfaceFb.setPropertyValue("PayloadType", canPayloadType);

    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    const StreamReaderPtr reader = StreamReaderSkipEvents(outputSignal, SampleType::Struct, SampleType::Int64);

    callsMultiMap.processPacket(canPacket);
    auto samplesCount = waitForSamples(reader);
    ASSERT_EQ(samplesCount, 0);

    canPacket->setInterfaceId(newInterfaceId);
    callsMultiMap.processPacket(canPacket);
    samplesCount = waitForSamples(reader);
    ASSERT_EQ(samplesCount, 1);
}

TEST_F(StreamFbTest, ChangeDeviceId)
{
    constexpr Int newDeviceId = 100;
    captureFb.setPropertyValue("DeviceId", newDeviceId);
    interfaceFb.setPropertyValue("PayloadType", canPayloadType);

    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    const StreamReaderPtr reader = StreamReaderSkipEvents(outputSignal, SampleType::Struct, SampleType::Int64);

    callsMultiMap.processPacket(canPacket);
    auto samplesCount = waitForSamples(reader);
    ASSERT_EQ(samplesCount, 0);

    canPacket->setDeviceId(newDeviceId);
    callsMultiMap.processPacket(canPacket);
    samplesCount = waitForSamples(reader);
    ASSERT_EQ(samplesCount, 1);
}

TEST_F(StreamFbTest, RemoveStream)
{
    interfaceFb.getPropertyValue("RemoveStream").execute(0);
    ASSERT_EQ(interfaceFb.getFunctionBlocks().getCount(), 0);

    ASSERT_EQ(callsMultiMap.size(), 0);
}

TEST_F(StreamFbTest, RemoveInterface)
{
    captureFb.getPropertyValue("RemoveInterface").execute(0);
    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 0);

    ASSERT_EQ(callsMultiMap.size(), 0);
}
