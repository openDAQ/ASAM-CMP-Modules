#include <asam_cmp_data_sink/calls_multi_map.h>
#include <asam_cmp_data_sink/capture_fb.h>
#include <asam_cmp_data_sink/common.h>
#include <asam_cmp_data_sink/data_handler.h>

#include <asam_cmp/analog_payload.h>
#include <asam_cmp/can_payload.h>
#include <asam_cmp/packet.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/module_ptr.h>
#include <opendaq/reader_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>

using namespace std::chrono_literals;

using namespace daq;
using ASAM::CMP::AnalogPayload;
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
    void SetUp() override
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
    }

protected:
    static constexpr uint16_t deviceId = 0;
    static constexpr uint32_t interfaceId = 1;
    static constexpr uint8_t streamId = 2;
    static constexpr uint64_t timeResolution = 1e9;

    static constexpr int canPayloadType = 1;
    static constexpr int analogPayloadType = 3;

protected:
    CallsMultiMap callsMultiMap;
    FunctionBlockPtr captureFb;
    FunctionBlockPtr interfaceFb;
    FunctionBlockPtr funcBlock;
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
    const StringPtr dataName = "Data";
    const StringPtr timeName = "Time";

    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    ASSERT_EQ(outputSignal.getName(), dataName);

    const auto domainSignal = outputSignal.getDomainSignal();
    ASSERT_TRUE(domainSignal.assigned());
    ASSERT_EQ(domainSignal.getName(), timeName);
}

TEST_F(StreamFbTest, DefaultSignalDescriptors)
{
    constexpr SampleType sampleType = SampleType::UInt64;
    const StringPtr timeName = "Time";

    const auto dataDescriptor = funcBlock.getSignalsRecursive()[0].getDescriptor();
    ASSERT_FALSE(dataDescriptor.assigned());

    const auto domainDescr = funcBlock.getSignalsRecursive()[0].getDomainSignal().getDescriptor();
    ASSERT_EQ(domainDescr.getSampleType(), sampleType);
    ASSERT_EQ(domainDescr.getRule(), ExplicitDataRule());
    ASSERT_EQ(domainDescr.getTickResolution(), Ratio(1, timeResolution));
    ASSERT_EQ(domainDescr.getName(), timeName);
}

TEST_F(StreamFbTest, RemoveStream)
{
    interfaceFb.getPropertyValue("RemoveStream").execute(0);
    ASSERT_EQ(interfaceFb.getFunctionBlocks().getCount(), 0);

    ASSERT_EQ(callsMultiMap.size(), 0);
}

TEST_F(StreamFbTest, RemoveAddStream)
{
    interfaceFb.getPropertyValue("RemoveStream").execute(0);
    interfaceFb.getPropertyValue("AddStream").execute();
    funcBlock = interfaceFb.getFunctionBlocks().getItemAt(0);

    funcBlock.setPropertyValue("StreamId", streamId);
    Int curStreamId = funcBlock.getPropertyValue("StreamId");
    ASSERT_EQ(curStreamId, streamId);
}

TEST_F(StreamFbTest, RemoveInterface)
{
    captureFb.getPropertyValue("RemoveInterface").execute(0);
    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 0);

    ASSERT_EQ(callsMultiMap.size(), 0);
}

class StreamFbCanPayloadTest : public StreamFbTest
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
    void SetUp() override
    {
        StreamFbTest::SetUp();

        createCanPacket();
    }

    std::shared_ptr<Packet> createCanPacket()
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

        return canPacket;
    }

protected:
    static constexpr uint32_t arbId = 45;

protected:
    std::shared_ptr<Packet> canPacket;
    const uint32_t canData = 33;
};

TEST_F(StreamFbCanPayloadTest, ChangeStreamId)
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

TEST_F(StreamFbCanPayloadTest, ChangeInterfaceId)
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

TEST_F(StreamFbCanPayloadTest, ChangeDeviceId)
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

TEST_F(StreamFbCanPayloadTest, CanSignalDescriptors)
{
    const StringPtr name = "CAN";
    constexpr SampleType sampleType = SampleType::Struct;
    constexpr SampleType domainSampleType = SampleType::UInt64;
    constexpr DataRuleType dataRuleType = DataRuleType::Explicit;

    interfaceFb.setPropertyValue("PayloadType", canPayloadType);

    const auto descriptor = funcBlock.getSignalsRecursive()[0].getDescriptor();
    ASSERT_EQ(descriptor.getName(), name);
    ASSERT_EQ(descriptor.getSampleType(), sampleType);
    ASSERT_EQ(descriptor.getSampleSize(), sizeof(CANData));
    ASSERT_FALSE(descriptor.getPostScaling().assigned());
    ASSERT_FALSE(descriptor.getUnit().assigned());

    const auto domainDescr = funcBlock.getSignalsRecursive()[0].getDomainSignal().getDescriptor();
    ASSERT_EQ(domainDescr.getRule().getType(), dataRuleType);
    ASSERT_EQ(domainDescr.getSampleType(), domainSampleType);
    ASSERT_EQ(domainDescr.getTickResolution(), Ratio(1, timeResolution));
}

TEST_F(StreamFbCanPayloadTest, ReceivePacketWithWrongPayloadType)
{
    interfaceFb.setPropertyValue("PayloadType", analogPayloadType);
    const auto dataHandler = funcBlock.as<IDataHandler>(true);

    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    const StreamReaderPtr reader = StreamReader(outputSignal);

    dataHandler->processData(canPacket);
    const bool haveSamples = waitForSamples(reader);
    ASSERT_FALSE(haveSamples);
}

TEST_F(StreamFbCanPayloadTest, ReadOutputCanSignal)
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

template <typename AnalogType>
class StreamFbAnalogPayloadTest : public StreamFbTest
{
protected:
    void SetUp() override
    {
        StreamFbTest::SetUp();

        analogPacket = createAnalogPacket<AnalogType>();
    }

    template <typename U>
    std::shared_ptr<Packet> createAnalogPacket()
    {
        std::vector<U> analogData(analogDataSize);
        std::iota(analogData.begin(), analogData.end(), 0);

        AnalogPayload payload;
        payload.setData(reinterpret_cast<uint8_t*>(analogData.data()), analogData.size() * sizeof(U));
        payload.setSampleDt(AnalogPayload::SampleDtFromType<U>::sampleDtType);
        payload.setUnit(AnalogPayload::Unit::kilogram);
        payload.setSampleInterval(sampleInterval);
        payload.setSampleOffset(sampleOffset);
        payload.setSampleScalar(sampleScalar);

        auto packet = std::make_shared<Packet>();
        packet->setPayload(payload);
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        packet->setTimestamp(timestamp);
        packet->setDeviceId(deviceId);
        packet->setInterfaceId(interfaceId);
        packet->setStreamId(streamId);

        return packet;
    }

    DataDescriptorPtr readDataDescriptor(const StreamReaderPtr& reader, const StringPtr& descriptorName)
    {
        ReaderStatusPtr status;
        DataDescriptorPtr descriptor;
        size_t count = 0;
        do
        {
            status = reader.read(nullptr, &count);
            EXPECT_EQ(status.getEventPacket().getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
            descriptor = status.getEventPacket().getParameters().get(descriptorName);
        } while (status.getReadStatus() == ReadStatus::Event && !descriptor.assigned());

        return descriptor;
    }

protected:
    static constexpr size_t analogDataSize = 80;
    static constexpr float sampleInterval = 20.f * 1e-6f;
    static constexpr float sampleOffset = 50;
    static constexpr float sampleScalar = 0.3f;
    static constexpr uint64_t deltaT = timeResolution * sampleInterval;

protected:
    std::shared_ptr<Packet> analogPacket;
};

using AnalogTypes = ::testing::Types<int16_t, int32_t>;
TYPED_TEST_SUITE(StreamFbAnalogPayloadTest, AnalogTypes);

TYPED_TEST(StreamFbAnalogPayloadTest, AnalogSignalDescriptor)
{
    const StringPtr name = "Analog";
    constexpr SampleType sampleType = SampleType::Float64;
    constexpr SampleType domainSampleType = SampleType::UInt64;
    const UnitPtr unit = Unit("kg", -1, "", "");

    interfaceFb.setPropertyValue("PayloadType", analogPayloadType);
    funcBlock.as<IDataHandler>(true)->processData(analogPacket);

    const auto descriptor = funcBlock.getSignalsRecursive()[0].getDescriptor();
    ASSERT_EQ(descriptor.getName(), name);
    ASSERT_EQ(descriptor.getSampleType(), sampleType);
    ASSERT_EQ(descriptor.getSampleSize(), sizeof(double));
    ASSERT_EQ(descriptor.getPostScaling().getType(), ScalingType::Linear);
    ASSERT_EQ(descriptor.getUnit(), unit);

    const auto domainDescr = funcBlock.getSignalsRecursive()[0].getDomainSignal().getDescriptor();
    ASSERT_EQ(domainDescr.getRule().getType(), DataRuleType::Linear);
    ASSERT_EQ(domainDescr.getSampleType(), domainSampleType);
    ASSERT_EQ(domainDescr.getTickResolution(), Ratio(1, timeResolution));
}

TYPED_TEST(StreamFbAnalogPayloadTest, ReadOutputAnalogSignal)
{
    interfaceFb.setPropertyValue("PayloadType", analogPayloadType);
    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    const StreamReaderPtr reader = StreamReaderSkipEvents(outputSignal, SampleType::Float64, SampleType::UInt64);

    const auto& analogPayload = static_cast<const AnalogPayload&>(analogPacket->getPayload());

    callsMultiMap.processPacket(analogPacket);
    auto samplesCount = waitForSamples(reader);
    ASSERT_EQ(samplesCount, analogPayload.getSamplesCount());

    std::vector<double> samples(samplesCount);
    std::vector<uint64_t> domainSample(samplesCount);
    size_t count = samplesCount;
    reader.readWithDomain(samples.data(), domainSample.data(), &count);
    ASSERT_EQ(count, samplesCount);

    std::vector<uint64_t> checkDomain(samplesCount);
    std::generate(checkDomain.begin(), checkDomain.end(), [t = analogPacket->getTimestamp() - deltaT]() mutable { return t += deltaT; });
    ASSERT_TRUE(std::equal(domainSample.begin(), domainSample.end(), checkDomain.begin()));

    auto analogData = reinterpret_cast<const TypeParam*>(analogPayload.getData());
    auto analogDataSize = analogPayload.getSamplesCount();

    std::vector<double> checkSamples(samplesCount);
    std::transform(
        analogData, analogData + analogDataSize, checkSamples.begin(), [](const double val) { return val * sampleScalar + sampleOffset; });
    ASSERT_TRUE(std::equal(samples.begin(), samples.end(), checkSamples.begin()));
}

template <typename T>
struct AnotherInt;

template <>
struct AnotherInt<int16_t>
{
    using type = int32_t;
};

template <>
struct AnotherInt<int32_t>
{
    using type = int16_t;
};

TYPED_TEST(StreamFbAnalogPayloadTest, DataTypeChanged)
{
    using AnotherType = AnotherInt<TypeParam>::type;

    interfaceFb.setPropertyValue("PayloadType", analogPayloadType);
    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    const StreamReaderPtr reader = StreamReaderSkipEvents(outputSignal, SampleType::Float64, SampleType::UInt64);

    callsMultiMap.processPacket(analogPacket);
    auto samplesCount = waitForSamples(reader);
    reader.skipSamples(&samplesCount);

    auto newPacket = createAnalogPacket<AnotherType>();
    newPacket->setTimestamp(analogPacket->getTimestamp() + samplesCount * deltaT);
    const auto& newPayload = static_cast<const AnalogPayload&>(newPacket->getPayload());
    callsMultiMap.processPacket(newPacket);
    samplesCount = waitForSamples(reader);
    ASSERT_EQ(samplesCount, newPayload.getSamplesCount());

    std::vector<double> samples(samplesCount);
    std::vector<uint64_t> domainSample(samplesCount);
    size_t count = samplesCount;
    reader.readWithDomain(samples.data(), domainSample.data(), &count);
    ASSERT_EQ(count, samplesCount);

    std::vector<uint64_t> checkDomain(samplesCount);
    std::generate(checkDomain.begin(), checkDomain.end(), [t = newPacket->getTimestamp() - deltaT]() mutable { return t += deltaT; });
    ASSERT_TRUE(std::equal(domainSample.begin(), domainSample.end(), checkDomain.begin()));

    auto analogData = reinterpret_cast<const AnotherType*>(newPayload.getData());
    auto analogDataSize = newPayload.getSamplesCount();

    std::vector<double> checkSamples(samplesCount);
    std::transform(
        analogData, analogData + analogDataSize, checkSamples.begin(), [](const double val) { return val * sampleScalar + sampleOffset; });
    ASSERT_TRUE(std::equal(samples.begin(), samples.end(), checkSamples.begin()));
}

TYPED_TEST(StreamFbAnalogPayloadTest, UnitChanged)
{
    constexpr auto asamCmpUnit = AnalogPayload::Unit::ampere;
    const StringPtr openDaqUnit = "A";

    interfaceFb.setPropertyValue("PayloadType", analogPayloadType);
    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    const StreamReaderPtr reader = StreamReader(outputSignal, SampleType::Float64, SampleType::UInt64);

    callsMultiMap.processPacket(analogPacket);
    size_t count = 0;
    while (reader.read(nullptr, &count).getReadStatus() == ReadStatus::Event)
        ;

    auto samplesCount = waitForSamples(reader);
    reader.skipSamples(&samplesCount);

    analogPacket->setTimestamp(analogPacket->getTimestamp() + samplesCount * deltaT);
    auto& payload = static_cast<AnalogPayload&>(analogPacket->getPayload());
    payload.setUnit(asamCmpUnit);
    callsMultiMap.processPacket(analogPacket);

    auto descriptor = readDataDescriptor(reader, "DataDescriptor");
    ASSERT_EQ(descriptor.getUnit().getSymbol(), openDaqUnit);
}

TYPED_TEST(StreamFbAnalogPayloadTest, SampleIntervalChanged)
{
    constexpr auto newSampleInterval = sampleInterval / 2;
    static constexpr uint64_t newDeltaT = timeResolution * newSampleInterval;

    interfaceFb.setPropertyValue("PayloadType", analogPayloadType);
    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    const StreamReaderPtr reader = StreamReader(outputSignal, SampleType::Float64, SampleType::UInt64);

    callsMultiMap.processPacket(analogPacket);
    size_t count = 0;
    while (reader.read(nullptr, &count).getReadStatus() == ReadStatus::Event)
        ;

    auto samplesCount = waitForSamples(reader);
    reader.skipSamples(&samplesCount);

    analogPacket->setTimestamp(analogPacket->getTimestamp() + samplesCount * deltaT);
    auto& payload = static_cast<AnalogPayload&>(analogPacket->getPayload());
    payload.setSampleInterval(newSampleInterval);
    callsMultiMap.processPacket(analogPacket);

    auto descriptor = readDataDescriptor(reader, "DomainDataDescriptor");
    ASSERT_EQ(descriptor.getRule().getParameters().get("delta"), newDeltaT);
}

TYPED_TEST(StreamFbAnalogPayloadTest, PostScalingChanged)
{
    constexpr float newSampleOffset = sampleOffset * 2;
    constexpr float newSampleScalar = sampleScalar * 2;

    interfaceFb.setPropertyValue("PayloadType", analogPayloadType);
    const auto outputSignal = funcBlock.getSignalsRecursive()[0];
    const StreamReaderPtr reader = StreamReader(outputSignal, SampleType::Float64, SampleType::UInt64);

    callsMultiMap.processPacket(analogPacket);
    size_t count = 0;
    while (reader.read(nullptr, &count).getReadStatus() == ReadStatus::Event)
        ;

    auto samplesCount = waitForSamples(reader);
    reader.skipSamples(&samplesCount);

    analogPacket->setTimestamp(analogPacket->getTimestamp() + samplesCount * deltaT);
    auto& payload = static_cast<AnalogPayload&>(analogPacket->getPayload());
    payload.setSampleOffset(newSampleOffset);
    payload.setSampleScalar(newSampleScalar);
    callsMultiMap.processPacket(analogPacket);

    auto descriptor = readDataDescriptor(reader, "DataDescriptor");
    ASSERT_EQ(descriptor.getPostScaling().getParameters().get("offset"), newSampleOffset);
    ASSERT_EQ(descriptor.getPostScaling().getParameters().get("scale"), newSampleScalar);
}

using StreamFbAnalogPayloadInt32Test = StreamFbAnalogPayloadTest<int32_t>;

TEST_F(StreamFbAnalogPayloadInt32Test, PostScalingChanged2)
{
    interfaceFb.getPropertyValue("RemoveStream").execute(0);
    interfaceFb.setPropertyValue("PayloadType", analogPayloadType);
    interfaceFb.getPropertyValue("AddStream").execute();
    funcBlock = interfaceFb.getFunctionBlocks().getItemAt(0);

    funcBlock.setPropertyValue("StreamId", streamId);
    ASSERT_NO_THROW(callsMultiMap.processPacket(analogPacket));
}
