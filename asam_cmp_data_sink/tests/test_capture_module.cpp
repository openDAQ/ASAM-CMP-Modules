#include <asam_cmp_data_sink/common.h>

#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>

#include <asam_cmp_data_sink/capture_fb.h>

using namespace daq;

class CaptureModuleTest : public testing::Test
{
protected:
    CaptureModuleTest()
    {
        auto logger = Logger();
        captureFb = createWithImplementation<IFunctionBlock, modules::asam_cmp_data_sink_module::CaptureFb>(
            Context(Scheduler(logger), logger, nullptr, nullptr, nullptr), nullptr, "capture_module_0", callsMultiMap);
    }

protected:
    modules::asam_cmp_data_sink_module::CallsMultiMap callsMultiMap;
    FunctionBlockPtr captureFb;
};

TEST_F(CaptureModuleTest, CreateCaptureModule)
{
    ASSERT_NE(captureFb, nullptr);
}

TEST_F(CaptureModuleTest, CaptureModuleProperties)
{
    ASSERT_TRUE(captureFb.hasProperty("DeviceId"));
    ASSERT_TRUE(captureFb.hasProperty("AddInterface"));
    ASSERT_TRUE(captureFb.hasProperty("RemoveInterface"));
}

TEST_F(CaptureModuleTest, TestCreateInterface)
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
