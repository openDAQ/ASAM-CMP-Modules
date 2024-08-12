#include <asam_cmp_data_sink/common.h>
#include <asam_cmp_data_sink/capture_fb.h>

#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>

using namespace daq;
using namespace testing;

class AsamCmpStreamTest : public testing::Test
{
protected:
    void SetUp()
    {
        auto logger = Logger();
        captureFb = createWithImplementation<IFunctionBlock, modules::asam_cmp_data_sink_module::CaptureFb>(
            Context(Scheduler(logger), logger, nullptr, nullptr, nullptr), nullptr, "capture_module_0", callsMultiMap);

        ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");
        createProc();

        interface = captureFb.getFunctionBlocks().getItemAt(0);
    }

protected:
    modules::asam_cmp_data_sink_module::CallsMultiMap callsMultiMap;
    FunctionBlockPtr captureFb;
    FunctionBlockPtr interface;
};
