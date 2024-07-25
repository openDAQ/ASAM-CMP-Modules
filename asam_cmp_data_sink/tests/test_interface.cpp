#include <asam_cmp_data_sink/common.h>
#include <asam_cmp_data_sink/capture_fb.h>

#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>

using namespace daq;
using namespace testing;

class AsamCmpInterfaceTest : public testing::Test
{
protected:
    AsamCmpInterfaceTest()
    {
        auto logger = Logger();
        captureFb = createWithImplementation<IFunctionBlock, modules::asam_cmp_data_sink_module::CaptureFb>(
            Context(Scheduler(logger), logger, nullptr, nullptr, nullptr), nullptr, "capture_module_0", callsMultiMap);

        ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");
        createProc();

        interfaceFb = captureFb.getFunctionBlocks().getItemAt(0);
    }

protected:
    FunctionBlockPtr captureFb;
    FunctionBlockPtr interfaceFb;
    modules::asam_cmp_data_sink_module::CallsMultiMap callsMultiMap;
};

TEST_F(AsamCmpInterfaceTest, CreateInterface)
{
    ASSERT_NE(interfaceFb, nullptr);
}

TEST_F(AsamCmpInterfaceTest, CaptureModuleProperties)
{

    ASSERT_TRUE(interfaceFb.hasProperty("InterfaceId"));
    ASSERT_TRUE(interfaceFb.hasProperty("PayloadType"));
    ASSERT_TRUE(interfaceFb.hasProperty("AddStream"));
    ASSERT_TRUE(interfaceFb.hasProperty("RemoveStream"));
}

TEST_F(AsamCmpInterfaceTest, TestSetId)
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
