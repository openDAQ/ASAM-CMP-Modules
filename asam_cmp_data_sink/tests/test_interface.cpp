#include <asam_cmp_data_sink/capture_fb.h>
#include <asam_cmp_data_sink/common.h>

#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>

using namespace daq;
using namespace testing;

class AsamCmpInterfaceFixture : public ::testing::Test
{
protected:
    AsamCmpInterfaceFixture()
    {
        auto logger = Logger();
        captureFb = createWithImplementation<IFunctionBlock, modules::asam_cmp_data_sink_module::CaptureFb>(
            Context(Scheduler(logger), logger, TypeManager(), nullptr), nullptr, "capture_module_0", callsMultiMap);

        captureFb.getPropertyValue("AddInterface").execute();
        interfaceFb = captureFb.getFunctionBlocks().getItemAt(0);
    }

protected:
    FunctionBlockPtr captureFb;
    FunctionBlockPtr interfaceFb;
    modules::asam_cmp_data_sink_module::CallsMultiMap callsMultiMap;
};

TEST_F(AsamCmpInterfaceFixture, NotNull)
{
    ASSERT_NE(interfaceFb, nullptr);
}

TEST_F(AsamCmpInterfaceFixture, CaptureModuleProperties)
{
    ASSERT_TRUE(interfaceFb.hasProperty("InterfaceId"));
    ASSERT_TRUE(interfaceFb.hasProperty("PayloadType"));
    ASSERT_TRUE(interfaceFb.hasProperty("AddStream"));
    ASSERT_TRUE(interfaceFb.hasProperty("RemoveStream"));
}

TEST_F(AsamCmpInterfaceFixture, InterfaceId)
{
    ProcedurePtr createProc = captureFb.getPropertyValue("AddInterface");
    createProc();

    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 2);

    FunctionBlockPtr interfaceFb2 = captureFb.getFunctionBlocks().getItemAt(1);

    uint32_t id1 = interfaceFb.getPropertyValue("InterfaceId"), id2 = interfaceFb2.getPropertyValue("InterfaceId");
    ASSERT_NE(id1, id2);

    interfaceFb2.setPropertyValue("InterfaceId", id1);
    ASSERT_EQ(interfaceFb2.getPropertyValue("InterfaceId"), id2);

    interfaceFb2.setPropertyValue("InterfaceId", static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 1);
    ASSERT_EQ(interfaceFb2.getPropertyValue("InterfaceId"), id2);

    interfaceFb2.setPropertyValue("InterfaceId", id2 + 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_EQ(interfaceFb2.getPropertyValue("InterfaceId"), id2 + 1);

    interfaceFb2.setPropertyValue("InterfaceId", id1);
    ASSERT_EQ(interfaceFb2.getPropertyValue("InterfaceId"), id2 + 1);

    interfaceFb2.setPropertyValue("InterfaceId", static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 1);
    ASSERT_EQ(interfaceFb2.getPropertyValue("InterfaceId"), id2 + 1);
}

TEST_F(AsamCmpInterfaceFixture, AddStream)
{
    ProcedurePtr addStream = interfaceFb.getPropertyValue("AddStream");
    interfaceFb.setPropertyValue("PayloadType", 1);
    addStream();
    addStream();

    auto s1 = interfaceFb.getFunctionBlocks().getItemAt(0), s2 = interfaceFb.getFunctionBlocks().getItemAt(1);

    ASSERT_NE(s1.getPropertyValue("StreamId"), s2.getPropertyValue("StreamId"));
}
