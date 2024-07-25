#include <asam_cmp_data_sink/common.h>

#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>

#include <asam_cmp_data_sink/capture_fb.h>

using CaptureModuleTest = testing::Test;
using namespace daq;

static FunctionBlockPtr createAsamCmpCapture()
{
    auto logger = Logger();
    FunctionBlockPtr captureModule = createWithImplementation<IFunctionBlock, modules::asam_cmp_data_sink_module::CaptureFb>(
        Context(Scheduler(logger), logger, nullptr, nullptr, nullptr), nullptr, "capture_module_0");

    return captureModule;
}

TEST_F(CaptureModuleTest, CreateCaptureModule)
{
    auto asamCmpCapture = createAsamCmpCapture();
    ASSERT_NE(asamCmpCapture, nullptr);
}

TEST_F(CaptureModuleTest, CaptureModuleProperties)
{
    auto asamCmpCapture = createAsamCmpCapture();

    ASSERT_TRUE(asamCmpCapture.hasProperty("DeviceId"));
    ASSERT_TRUE(asamCmpCapture.hasProperty("AddInterface"));
    ASSERT_TRUE(asamCmpCapture.hasProperty("RemoveInterface"));
}

TEST_F(CaptureModuleTest, TestCreateInterface)
{
    auto asamCmpCapture = createAsamCmpCapture();

    ProcedurePtr createProc = asamCmpCapture.getPropertyValue("AddInterface");

    ASSERT_EQ(asamCmpCapture.getFunctionBlocks().getCount(), 0);
    createProc();
    createProc();
    ASSERT_EQ(asamCmpCapture.getFunctionBlocks().getCount(), 2);

    int lstId = asamCmpCapture.getFunctionBlocks().getItemAt(1).getPropertyValue("InterfaceId");

    ProcedurePtr removeProc = asamCmpCapture.getPropertyValue("RemoveInterface");
    removeProc(0);

    ASSERT_EQ(asamCmpCapture.getFunctionBlocks().getCount(), 1);
    ASSERT_EQ(asamCmpCapture.getFunctionBlocks().getItemAt(0).getPropertyValue("InterfaceId"), lstId);
}
