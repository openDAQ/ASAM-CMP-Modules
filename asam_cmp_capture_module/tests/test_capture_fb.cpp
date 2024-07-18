#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/module_dll.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <gtest/gtest.h>

using CaptureFbTest = testing::Test;
using namespace daq;

static FunctionBlockPtr createAsamCmpCapture()
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, Context(Scheduler(logger), logger, nullptr, nullptr, nullptr));

    auto fb = module.createFunctionBlock("asam_cmp_capture_module_fb", nullptr, "id");
    auto captureModule = fb.getFunctionBlocks().getItemAt(0);

    return captureModule;
}

TEST_F(CaptureFbTest, CreateCaptureModule)
{
    auto asamCmpCapture = createAsamCmpCapture();
    ASSERT_NE(asamCmpCapture, nullptr);
}

TEST_F(CaptureFbTest, CaptureModuleProperties)
{
    auto asamCmpCapture = createAsamCmpCapture();

    ASSERT_TRUE(asamCmpCapture.hasProperty("DeviceId"));
    ASSERT_TRUE(asamCmpCapture.hasProperty("AddInterface"));
    ASSERT_TRUE(asamCmpCapture.hasProperty("RemoveInterface"));
}

TEST_F(CaptureFbTest, TestCreateInterface)
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
