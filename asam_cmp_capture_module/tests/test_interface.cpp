#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/module_dll.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <gtest/gtest.h>

using AsamCmpInterfaceTest = testing::Test;
using namespace daq;

static FunctionBlockPtr createAsamCmpInterface()
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, Context(Scheduler(logger), logger, nullptr, nullptr, nullptr));

    auto fb = module.createFunctionBlock("asam_cmp_capture_module_fb", nullptr, "id");
    auto captureModule = fb.getFunctionBlocks().getItemAt(0);

    ProcedurePtr createProc = captureModule.getPropertyValue("AddInterface");
    createProc();

    return captureModule.getFunctionBlocks().getItemAt(0);
}

TEST_F(AsamCmpInterfaceTest, CreateInterface)
{
    auto asamCmpCapture = createAsamCmpInterface();
    ASSERT_NE(asamCmpCapture, nullptr);
}

TEST_F(AsamCmpInterfaceTest, CaptureModuleProperties)
{
    auto asamCmpCapture = createAsamCmpInterface();

    ASSERT_TRUE(asamCmpCapture.hasProperty("InterfaceId"));
    ASSERT_TRUE(asamCmpCapture.hasProperty("PayloadType"));
    ASSERT_TRUE(asamCmpCapture.hasProperty("AddStream"));
    ASSERT_TRUE(asamCmpCapture.hasProperty("RemoveStream"));
}

TEST_F(AsamCmpInterfaceTest, TestSetId)
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, Context(Scheduler(logger), logger, nullptr, nullptr, nullptr));

    auto fb = module.createFunctionBlock("asam_cmp_capture_module_fb", nullptr, "id");
    auto captureModule = fb.getFunctionBlocks().getItemAt(0);

    ProcedurePtr createProc = captureModule.getPropertyValue("AddInterface");
    createProc();
    createProc();

    FunctionBlockPtr itf1 = captureModule.getFunctionBlocks().getItemAt(0), itf2 = captureModule.getFunctionBlocks().getItemAt(1);

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
