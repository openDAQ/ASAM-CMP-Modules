#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/capture_fb.h>
#include <asam_cmp_capture_module/interface_fb.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <gtest/gtest.h>

using namespace daq;

class InterfaceFbTest: public testing::Test
{
    protected:
    void SetUp() {
        auto logger = Logger();
        context = Context(Scheduler(logger), logger, nullptr, nullptr, nullptr);
        const StringPtr captureModuleId = "asam_cmp_capture_fb";
        captureFb = createWithImplementation<IFunctionBlock, modules::asam_cmp_capture_module::CaptureFb>(
            context, nullptr, captureModuleId);

        ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");
        createProc();

        interfaceFb = captureFb.getFunctionBlocks().getItemAt(0);
    }
protected:
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
