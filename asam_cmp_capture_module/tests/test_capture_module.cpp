#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/module_dll.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>

using CaptureModuleTest = testing::Test;
using namespace daq;

static FunctionBlockPtr createAsamCmpCapture()
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, Context(Scheduler(logger), logger, nullptr, nullptr, nullptr));

    FunctionBlockPtr captureModule, fb;
    auto fbs = module.getAvailableFunctionBlockTypes();
    if (fbs.hasKey("asam_cmp_capture"))
    {
        fb = module.createFunctionBlock("asam_cmp_capture", nullptr, "id");
    }
    else
    {
        fb = module.createFunctionBlock("asam_cmp_data_sink_module", nullptr, "id");
        auto dataSink = fb.getFunctionBlocks(search::Recursive(search::LocalId("asam_cmp_data_sink")))[0];
        dataSink.getPropertyValue("AddCaptureModuleEmpty").execute();
    }

    captureModule = fb.getFunctionBlocks(search::Recursive(search::LocalId("capture_module")))[0];

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
