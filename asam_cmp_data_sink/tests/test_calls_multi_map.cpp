#include <coretypes/intfs.h>
#include <gmock/gmock.h>

#include <asam_cmp_data_sink/calls_multi_map.h>

using namespace daq;

using ASAM::CMP::Packet;
using daq::modules::asam_cmp_data_sink_module::CallsMultiMap;
using daq::modules::asam_cmp_data_sink_module::IDataHandler;

using DataHandlerImpl = ImplementationOf<IDataHandler>;

struct DataHandlerMock : public DataHandlerImpl
{
    MOCK_METHOD((void), processData, (const std::shared_ptr<ASAM::CMP::Packet>& packet), (override));
    MOCK_METHOD((void), processData, (const std::vector<std::shared_ptr<ASAM::CMP::Packet>>& packets), (override));
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
    CallsMultiMap callsMap;
    std::shared_ptr<Packet> packet;
};

TEST_F(CallsMultiMapTest, ProcessPacket)
{
    DataHandlerMock handler;
    callsMap.insert(deviceId, interfaceId, streamId, &handler);

    EXPECT_CALL(handler, processData(packet));
    callsMap.processPacket(packet);
}

TEST_F(CallsMultiMapTest, Process2Packets)
{
    constexpr int16_t deviceId2 = deviceId + 1;

    DataHandlerMock handler1, handler2;
    callsMap.insert(deviceId, interfaceId, streamId, &handler1);
    callsMap.insert(deviceId2, interfaceId, streamId, &handler2);

    EXPECT_CALL(handler1, processData(packet));
    callsMap.processPacket(packet);

    packet->setDeviceId(deviceId2);
    EXPECT_CALL(handler2, processData(packet));
    callsMap.processPacket(packet);
}

TEST_F(CallsMultiMapTest, ProcessWrongPacket)
{
    constexpr uint32_t wrongInterfaceId = interfaceId + 1;

    DataHandlerMock handler;
    callsMap.insert(deviceId, interfaceId, streamId, &handler);

    packet->setInterfaceId(wrongInterfaceId);

    EXPECT_CALL(handler, processData(packet)).Times(0);
    callsMap.processPacket(packet);
}

TEST_F(CallsMultiMapTest, ProcessPackets)
{
    DataHandlerMock handler;
    callsMap.insert(deviceId, interfaceId, streamId, &handler);

    std::vector<std::shared_ptr<Packet>> packets;
    packets.push_back(packet);
    packets.push_back(packet);
    packets.push_back(packet);
    EXPECT_CALL(handler, processData(packets));
    callsMap.processPackets(packets);
}

TEST_F(CallsMultiMapTest, SamePacketMultipleHandler)
{
    DataHandlerMock handler1, handler2;
    callsMap.insert(deviceId, interfaceId, streamId, &handler1);
    callsMap.insert(deviceId, interfaceId, streamId, &handler2);

    EXPECT_CALL(handler1, processData(packet));
    EXPECT_CALL(handler2, processData(packet));
    callsMap.processPacket(packet);
}

TEST_F(CallsMultiMapTest, Erase)
{
    DataHandlerMock handler1, handler2;
    callsMap.insert(deviceId, interfaceId, streamId, &handler1);
    callsMap.insert(deviceId, interfaceId, streamId, &handler2);

    callsMap.erase(deviceId, interfaceId, streamId, &handler1);

    EXPECT_CALL(handler1, processData(packet)).Times(0);
    EXPECT_CALL(handler2, processData(packet));
    callsMap.processPacket(packet);
}
