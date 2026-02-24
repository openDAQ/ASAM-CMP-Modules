#include <coretypes/intfs.h>
#include <gmock/gmock.h>

#include <asam_cmp_data_sink/data_packets_publisher.h>
#include <Packet.h>

using namespace daq;

using ASAM::CMP::Packet;
using daq::modules::asam_cmp_data_sink_module::DataPacketsPublisher;
using daq::modules::asam_cmp_data_sink_module::IAsamCmpPacketsSubscriber;

using DataHandlerImpl = ImplementationOf<IAsamCmpPacketsSubscriber>;

struct DataHandlerMock : public DataHandlerImpl
{
    MOCK_METHOD((void), receive, (const std::shared_ptr<ASAM::CMP::Packet>& packet), (override));
    MOCK_METHOD((void), receive, (const std::vector<std::shared_ptr<ASAM::CMP::Packet>>& packets), (override));
};

class CallsMultiMapTest : public testing::Test
{
protected:
    CallsMultiMapTest()
    {
        packet = std::make_shared<Packet>();
        packet->setDeviceId(deviceId);
        packet->setInterfaceId(interfaceId);
        packet->setStreamId(streamId);
    }

protected:
    static constexpr uint16_t deviceId = 0;
    static constexpr uint32_t interfaceId = 1;
    static constexpr uint8_t streamId = 2;

protected:
    DataPacketsPublisher publisher;
    std::shared_ptr<Packet> packet;
};

TEST_F(CallsMultiMapTest, ProcessPacket)
{
    DataHandlerMock handler;
    publisher.subscribe({deviceId, interfaceId, streamId}, &handler);

    EXPECT_CALL(handler, receive(packet));
    publisher.publish({packet->getDeviceId(), packet->getInterfaceId(), packet->getStreamId()}, packet);
}

TEST_F(CallsMultiMapTest, Process2Packets)
{
    constexpr int16_t deviceId2 = deviceId + 1;

    DataHandlerMock handler1, handler2;
    publisher.subscribe({deviceId, interfaceId, streamId}, &handler1);
    publisher.subscribe({deviceId2, interfaceId, streamId}, &handler2);

    EXPECT_CALL(handler1, receive(packet));
    publisher.publish({packet->getDeviceId(), packet->getInterfaceId(), packet->getStreamId()}, packet);

    packet->setDeviceId(deviceId2);
    EXPECT_CALL(handler2, receive(packet));
    publisher.publish({packet->getDeviceId(), packet->getInterfaceId(), packet->getStreamId()}, packet);
}

TEST_F(CallsMultiMapTest, ProcessWrongPacket)
{
    constexpr uint32_t wrongInterfaceId = interfaceId + 1;

    DataHandlerMock handler;
    publisher.subscribe({deviceId, interfaceId, streamId}, &handler);

    packet->setInterfaceId(wrongInterfaceId);

    EXPECT_CALL(handler, receive(packet)).Times(0);
    publisher.publish({packet->getDeviceId(), packet->getInterfaceId(), packet->getStreamId()}, packet);
}

TEST_F(CallsMultiMapTest, ProcessPackets)
{
    DataHandlerMock handler;
    publisher.subscribe({deviceId, interfaceId, streamId}, &handler);

    std::vector<std::shared_ptr<Packet>> packets;
    packets.push_back(packet);
    packets.push_back(packet);
    packets.push_back(packet);
    EXPECT_CALL(handler, receive(packets));
    publisher.publish({packet->getDeviceId(), packet->getInterfaceId(), packet->getStreamId()}, packets);
}

TEST_F(CallsMultiMapTest, SamePacketMultipleHandler)
{
    DataHandlerMock handler1, handler2;
    publisher.subscribe({deviceId, interfaceId, streamId}, &handler1);
    publisher.subscribe({deviceId, interfaceId, streamId}, &handler2);

    EXPECT_CALL(handler1, receive(packet));
    EXPECT_CALL(handler2, receive(packet));
    publisher.publish({packet->getDeviceId(), packet->getInterfaceId(), packet->getStreamId()}, packet);
}

TEST_F(CallsMultiMapTest, Erase)
{
    DataHandlerMock handler1, handler2;
    publisher.subscribe({deviceId, interfaceId, streamId}, &handler1);
    publisher.subscribe({deviceId, interfaceId, streamId}, &handler2);

    publisher.unsubscribe({deviceId, interfaceId, streamId}, &handler1);

    EXPECT_CALL(handler1, receive(packet)).Times(0);
    EXPECT_CALL(handler2, receive(packet));
    publisher.publish({packet->getDeviceId(), packet->getInterfaceId(), packet->getStreamId()}, packet);
}
