#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/capture_fb.h>
#include <asam_cmp_capture_module/interface_fb.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <gtest/gtest.h>

#include <asam_cmp_common_lib/ethernet_pcpp_mock.h>

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
        ON_CALL(*ethernetWrapper, sendPacket(_, _)).WillByDefault(WithArgs<0, 1>(Invoke(sendPacketStub)));

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
