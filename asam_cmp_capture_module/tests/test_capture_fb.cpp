#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/capture_fb.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <gtest/gtest.h>

using namespace daq;

class CaptureFbTest : public testing::Test
{
    protected:
    CaptureFbTest(){
        auto logger = Logger();
        context = Context(Scheduler(logger), logger, nullptr, nullptr, nullptr);
        const StringPtr captureModuleId = "asam_cmp_capture_fb";
        captureFb = createWithImplementation<IFunctionBlock, modules::asam_cmp_capture_module::CaptureFb>(
            context, nullptr, captureModuleId);
    }
protected:
    ContextPtr context;
    FunctionBlockPtr captureFb;
};

TEST_F(CaptureFbTest, CreateCaptureModule)
{
    ASSERT_NE(captureFb, nullptr);
}

TEST_F(CaptureFbTest, CaptureModuleProperties)
{
    ASSERT_TRUE(captureFb.hasProperty("DeviceId"));
    ASSERT_TRUE(captureFb.hasProperty("AddInterface"));
    ASSERT_TRUE(captureFb.hasProperty("RemoveInterface"));
}

TEST_F(CaptureFbTest, TestCreateInterface)
{
    ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");

    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 0);
    createProc();
    createProc();
    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 2);

    int lstId = captureFb.getFunctionBlocks().getItemAt(1).getPropertyValue("InterfaceId");

    ProcedurePtr removeProc = captureFb.getPropertyValue("RemoveInterface");
    removeProc(0);

    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 1);
    ASSERT_EQ(captureFb.getFunctionBlocks().getItemAt(0).getPropertyValue("InterfaceId"), lstId);
}
