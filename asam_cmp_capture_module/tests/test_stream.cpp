#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/module_dll.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <gtest/gtest.h>

using namespace daq;

class StreamFbTest: public testing::Test
{
    protected:
    void SetUp() {
        auto logger = Logger();
        createModule(&module, Context(Scheduler(logger), logger, nullptr, nullptr, nullptr));

        rootFb = module.createFunctionBlock("asam_cmp_capture_module_fb", nullptr, "id");
        captureModule = rootFb.getFunctionBlocks().getItemAt(0);

        ProcedurePtr createProc = captureModule.getPropertyValue("AddInterface");
        createProc();
        interface =  captureModule.getFunctionBlocks().getItemAt(0);
    }
protected:
    ModulePtr module;
    FunctionBlockPtr rootFb;
    FunctionBlockPtr captureModule;
    FunctionBlockPtr interface;
};

TEST_F(StreamFbTest, CreateStream)
{
    ProcedurePtr createProc = interface.getPropertyValue("AddStream");
    interface.setPropertyValue("PayloadType", 1);
    createProc();
    createProc();

    auto s1 = interface.getFunctionBlocks().getItemAt(0), s2 = interface.getFunctionBlocks().getItemAt(1);

    ASSERT_NE(s1.getPropertyValue("StreamId"), s2.getPropertyValue("StreamId"));
}
