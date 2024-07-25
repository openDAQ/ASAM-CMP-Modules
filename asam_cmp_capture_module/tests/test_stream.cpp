#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/capture_fb.h>
#include <asam_cmp_capture_module/interface_fb.h>
#include <asam_cmp_capture_module/stream_fb.h>
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

TEST_F(StreamFbTest, CreateStream)
{
    ProcedurePtr createProc = interfaceFb.getPropertyValue("AddStream");
    interfaceFb.setPropertyValue("PayloadType", 1);
    createProc();
    createProc();

    auto s1 = interfaceFb.getFunctionBlocks().getItemAt(0), s2 = interfaceFb.getFunctionBlocks().getItemAt(1);

    ASSERT_NE(s1.getPropertyValue("StreamId"), s2.getPropertyValue("StreamId"));
}
