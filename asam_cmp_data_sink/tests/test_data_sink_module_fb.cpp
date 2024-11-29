#include <EthLayer.h>
#include <PayloadLayer.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/scheduler_factory.h>

#include <asam_cmp_common_lib/ethernet_pcpp_mock.h>
#include <asam_cmp_common_lib/network_manager_fb.h>
#include <asam_cmp_data_sink/data_sink_module_fb.h>
#include <asam_cmp_data_sink/module_dll.h>

using namespace daq;
using daq::asam_cmp_common_lib::PcppPacketReceivedCallbackType;

using ::testing::Return;
using namespace testing;

class DataSinkModuleFbTest : public ::testing::Test
{
protected:
    DataSinkModuleFbTest()
    {
        ethernetWrapper = std::make_shared<asam_cmp_common_lib::EthernetPcppMock>();

        names = List<IString>();
        names.pushBack("name1");
        names.pushBack("name2");

        descriptions = List<IString>();
        descriptions.pushBack("desc1");
        descriptions.pushBack("desc2");

        auto startStub = [this](PcppPacketReceivedCallbackType callback)
        { packetReceivedCallback = callback; };

        auto stopStub = []() {};

        ON_CALL(*ethernetWrapper, startCapture(_)).WillByDefault(startStub);
        ON_CALL(*ethernetWrapper, stopCapture()).WillByDefault(stopStub);
        ON_CALL(*ethernetWrapper, getEthernetDevicesNamesList()).WillByDefault(Return(names));
        ON_CALL(*ethernetWrapper, getEthernetDevicesDescriptionsList()).WillByDefault(Return(descriptions));

        EXPECT_CALL(*ethernetWrapper, startCapture(_)).Times(AtLeast(1));
        EXPECT_CALL(*ethernetWrapper, stopCapture()).Times(AtLeast(1));
        EXPECT_CALL(*ethernetWrapper, setDevice(_)).Times(AtLeast(1)).WillRepeatedly(Return(true));
        EXPECT_CALL(*ethernetWrapper, getEthernetDevicesNamesList()).Times(AtLeast(1));
        EXPECT_CALL(*ethernetWrapper, getEthernetDevicesDescriptionsList()).Times(AtLeast(1));

        auto logger = Logger();
        context = Context(Scheduler(logger), logger, TypeManager(), nullptr);

        funcBlock = createWithImplementation<IFunctionBlock, modules::asam_cmp_data_sink_module::DataSinkModuleFb>(
            context, nullptr, "id", ethernetWrapper);
    }

protected:
    template <typename T>
    void testProperty(const StringPtr& name, T newValue, bool success = true);

protected:
    static constexpr std::string_view networkAdapters = "NetworkAdapters";

protected:
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppMock> ethernetWrapper;
    ListPtr<StringPtr> names;
    ListPtr<StringPtr> descriptions;

    ContextPtr context;
    FunctionBlockPtr funcBlock;

    PcppPacketReceivedCallbackType packetReceivedCallback;
};

TEST_F(DataSinkModuleFbTest, NotNull)
{
    ASSERT_NE(ethernetWrapper, nullptr);
    ASSERT_NE(context, nullptr);
    ASSERT_NE(funcBlock, nullptr);
}

TEST_F(DataSinkModuleFbTest, FunctionBlockType)
{
    auto type = funcBlock.getFunctionBlockType();
    ASSERT_EQ(type.getId(), "AsamCmpDataSinkModule");
    ASSERT_EQ(type.getName(), "AsamCmpDataSinkModule");
    ASSERT_EQ(type.getDescription(), "ASAM CMP Data Sink Module");
}

template <typename T>
void DataSinkModuleFbTest::testProperty(const StringPtr& name, T newValue, bool success)
{
    funcBlock.setPropertyValue(name, newValue);
    const T value = funcBlock.getPropertyValue(name);
    if (success)
        ASSERT_EQ(value, newValue);
    else
        ASSERT_NE(value, newValue);
}

TEST_F(DataSinkModuleFbTest, NetworkAdaptersProperties)
{
    auto propList = funcBlock.getProperty(networkAdapters.data()).getSelectionValues().asPtrOrNull<IList>();
    ASSERT_EQ(propList.getCount(), descriptions.getCount());
    ASSERT_TRUE(std::equal(propList.begin(), propList.end(), descriptions.begin()));
}

TEST_F(DataSinkModuleFbTest, ChangeNetworkAdapter)
{
    auto propList = funcBlock.getProperty(networkAdapters.data()).getSelectionValues().asPtrOrNull<IList>();
    ASSERT_GT(propList.getCount(), 1);
    constexpr int newVal = 1;
    testProperty(networkAdapters.data(), newVal);
}

TEST_F(DataSinkModuleFbTest, ChangeNetworkAdapterSequenceCall)
{
    {
        InSequence inSeq;
        EXPECT_CALL(*ethernetWrapper, stopCapture()).Times(Exactly(1));
        EXPECT_CALL(*ethernetWrapper, setDevice(_)).Times(Exactly(1)).WillRepeatedly(Return(true));
    }

    ASSERT_GT(descriptions.getCount(), 1);
    constexpr int newVal = 1;
    testProperty(networkAdapters.data(), newVal);
}

TEST_F(DataSinkModuleFbTest, NestedFbCount)
{
    EXPECT_EQ(funcBlock.getFunctionBlocks().getCount(), 2u);
}

TEST_F(DataSinkModuleFbTest, ProcessAggregatedMessage)
{
    constexpr uint16_t asamCmpEtherType = 0x99FE;
    constexpr int canFdPayloadType = 2;
    constexpr size_t messagesCount = 5;

    const std::vector<uint8_t> ethData = {
        0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x17, 0xef, 0xf0, 0xeb, 0x13, 0x6b, 0xc1, 0x18, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02,
        0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x17, 0xef, 0xf0, 0xeb, 0x16, 0x12, 0xcd, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x20, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x10, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
        0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0x17, 0xef, 0xf0, 0xeb, 0x18, 0xb9, 0xd8, 0xf8, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02,
        0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x02, 0x00, 0x00, 0x00,
        0xfe, 0xff, 0xff, 0xff, 0x17, 0xef, 0xf0, 0xeb, 0x1b, 0x60, 0xe4, 0xe8, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x20, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x10, 0x03, 0x00, 0x00, 0x00, 0xfd, 0xff, 0xff, 0xff,
        0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0x17, 0xef, 0xf0, 0xeb, 0x1e, 0x07, 0xf0, 0xd8, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02,
        0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x04, 0x00, 0x00, 0x00,
        0xfc, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1a, 0x44, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x44, 0x65, 0x76,
        0x69, 0x63, 0x65, 0x44, 0x65, 0x73, 0x63, 0x72, 0x69, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x00, 0x00, 0x14, 0x44, 0x65, 0x66, 0x61,
        0x75, 0x6c, 0x74, 0x53, 0x65, 0x72, 0x61, 0x69, 0x6c, 0x4e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x00, 0x00, 0x18, 0x44, 0x65, 0x66, 0x61,
        0x75, 0x6c, 0x74, 0x48, 0x61, 0x72, 0x64, 0x77, 0x77, 0x61, 0x72, 0x65, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x00, 0x00, 0x18,
        0x44, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x53, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x01, 0x00, 0x00, 0x00};

    auto dataSinkFb = funcBlock.getFunctionBlocks().getItemAt(1);
    dataSinkFb.getPropertyValue("AddCaptureModuleEmpty").execute();
    auto captureFb = dataSinkFb.getFunctionBlocks().getItemAt(0);
    captureFb.getPropertyValue("AddInterface").execute();
    auto interfaceFb = captureFb.getFunctionBlocks().getItemAt(0);
    interfaceFb.getPropertyValue("AddStream").execute();
    interfaceFb.setPropertyValue("PayloadType", canFdPayloadType);
    auto streamFb = interfaceFb.getFunctionBlocks().getItemAt(0);
    PacketReaderPtr reader = PacketReader(streamFb.getSignals()[0]);

    // create an Ethernet packet
    pcpp::EthLayer newEthernetLayer(pcpp::MacAddress("00:50:43:11:22:33"), pcpp::MacAddress("FF:FF:FF:FF:FF:FF"), asamCmpEtherType);
    pcpp::PayloadLayer payloadLayer(ethData.data(), ethData.size());
    pcpp::Packet newPacket;
    bool res = newPacket.addLayer(&newEthernetLayer);
    res = newPacket.addLayer(&payloadLayer);
    newPacket.computeCalculateFields();

    packetReceivedCallback(newPacket.getRawPacket(), nullptr, nullptr);

    auto packet = reader.read();
    ASSERT_NE(packet, nullptr);
    ASSERT_EQ(packet.getType(), PacketType::Event);

    packet = reader.read();
    ASSERT_EQ(packet.getType(), PacketType::Data);
    DataPacketPtr dataPacket = packet;

    const size_t sampleCount = dataPacket.getSampleCount();
    ASSERT_EQ(sampleCount, messagesCount);
}
