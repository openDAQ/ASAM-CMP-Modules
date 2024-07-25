#include <asam_cmp/capture_module_payload.h>
#include <asam_cmp/interface_payload.h>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>

#include <asam_cmp_data_sink/status_handler.h>
#include <asam_cmp_data_sink/status_fb_impl.h>

using namespace daq;

using ASAM::CMP::CaptureModulePayload;
using ASAM::CMP::InterfacePayload;
using ASAM::CMP::Packet;
using daq::modules::asam_cmp_data_sink_module::IStatusHandler;
using daq::modules::asam_cmp_data_sink_module::StatusMt;

class StatusFbFixture : public ::testing::Test
{
protected:
    StatusFbFixture()
    {
        auto logger = Logger();
        funcBlock = createWithImplementation<IFunctionBlock, modules::asam_cmp_data_sink_module::StatusFbImpl>(
            Context(Scheduler(logger), logger, TypeManager(), nullptr), nullptr, "asam_cmp_status");

        statusMt = std::make_unique<StatusMt>(funcBlock.asPtr<IStatusHandler>(true)->getStatusMt());
        cmPayload.setData(deviceDescr, "", "", "", {});
        ifPayload.setData(streams.data(), static_cast<uint16_t>(streams.size()), nullptr, 0);

        packet = std::make_shared<Packet>();
        packet->setPayload(cmPayload);
        packet->setVersion(1);
        packet->setDeviceId(deviceId);
    }

protected:
    bool checkDescription(const StringPtr& description, std::string_view deviceDescr, const uint16_t deviceId, size_t interfacesCount);

protected:
    FunctionBlockPtr funcBlock;

    CaptureModulePayload cmPayload;
    std::shared_ptr<Packet> packet;
    uint16_t deviceId = 3;
    std::string_view deviceDescr = "Device 3";
    std::unique_ptr<StatusMt> statusMt;
    InterfacePayload ifPayload;
    std::vector<uint8_t> streams = {1};
};

TEST_F(StatusFbFixture, NotNull)
{
    ASSERT_NE(funcBlock, nullptr);
}

TEST_F(StatusFbFixture, FunctionBlockType)
{
    auto type = funcBlock.getFunctionBlockType();
    ASSERT_EQ(type.getId(), "asam_cmp_status");
    ASSERT_EQ(type.getName(), "AsamCmpStatus");
    ASSERT_EQ(type.getDescription(), "ASAM CMP Status");
}

TEST_F(StatusFbFixture, CaptureModuleList)
{
    ListPtr<IString> cmList = funcBlock.getPropertyValue("CaptureModuleList");
    ASSERT_EQ(cmList.getCount(), 0);
    EXPECT_THROW(funcBlock.setPropertyValue("CaptureModuleList", cmList), daq::AccessDeniedException);
}

bool StatusFbFixture::checkDescription(const StringPtr& description,
                                              std::string_view deviceDescr,
                                              const uint16_t deviceId,
                                              size_t interfacesCount)
{
    StringPtr correctDescription = fmt::format("Id: {}, Name: {}, Interfaces: {}", deviceId, deviceDescr, interfacesCount);
    return description == correctDescription;
}

TEST_F(StatusFbFixture, ProcessStatusPackets)
{
    funcBlock.asPtr<IStatusHandler>(true)->processStatusPacket(packet);
    ListPtr<IString> cmList = funcBlock.getPropertyValue("CaptureModuleList");
    ASSERT_EQ(cmList.getCount(), 1u);
    ASSERT_EQ(statusMt->getStatus().getDeviceStatusCount(), 1u);
    ASSERT_TRUE(checkDescription(cmList[0], deviceDescr, deviceId, 0));

    deviceId = 2;
    cmPayload.setData(deviceDescr, "", "", "", {});
    packet->setPayload(cmPayload);
    packet->setDeviceId(deviceId);
    funcBlock.asPtr<IStatusHandler>(true)->processStatusPacket(packet);
    cmList = funcBlock.getPropertyValue("CaptureModuleList");
    ASSERT_EQ(cmList.getCount(), 2u);
    ASSERT_EQ(statusMt->getStatus().getDeviceStatusCount(), 2u);
    ASSERT_TRUE(checkDescription(cmList[1], deviceDescr, deviceId, 0));

    deviceId = 3;
    packet->setPayload(ifPayload);
    packet->setDeviceId(deviceId);
    funcBlock.asPtr<IStatusHandler>(true)->processStatusPacket(packet);
    cmList = funcBlock.getPropertyValue("CaptureModuleList");
    ASSERT_EQ(cmList.getCount(), 2u);
    ASSERT_EQ(statusMt->getStatus().getDeviceStatusCount(), 2u);
    ASSERT_TRUE(checkDescription(cmList[0], deviceDescr, deviceId, 1));

    deviceId = 5;
    packet->setDeviceId(deviceId);
    funcBlock.asPtr<IStatusHandler>(true)->processStatusPacket(packet);
    cmList = funcBlock.getPropertyValue("CaptureModuleList");
    ASSERT_EQ(cmList.getCount(), 2u);
    ASSERT_EQ(statusMt->getStatus().getDeviceStatusCount(), 2u);
}

TEST_F(StatusFbFixture, Clear)
{
    funcBlock.asPtr<IStatusHandler>(true)->processStatusPacket(packet);
    ListPtr<IString> cmList = funcBlock.getPropertyValue("CaptureModuleList");
    ASSERT_EQ(cmList.getCount(), 1u);
    ASSERT_EQ(statusMt->getStatus().getDeviceStatusCount(), 1u);

    auto proc = daq::Procedure([]() {});
    EXPECT_THROW(funcBlock.setPropertyValue("Clear", proc), daq::AccessDeniedException);

    funcBlock.getPropertyValue("Clear").execute();
    cmList = funcBlock.getPropertyValue("CaptureModuleList");
    ASSERT_EQ(cmList.getCount(), 0u);
    ASSERT_EQ(statusMt->getStatus().getDeviceStatusCount(), 0u);
}
