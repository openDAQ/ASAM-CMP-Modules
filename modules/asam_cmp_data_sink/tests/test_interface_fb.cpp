#include <asam_cmp_data_sink/capture_fb.h>
#include <asam_cmp_data_sink/stream_fb.h>
#include <asam_cmp_data_sink/common.h>

#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>

#include <thread>

using namespace daq;
using namespace testing;

class InterfaceFbTest : public ::testing::Test
{
protected:
    InterfaceFbTest()
    {
        auto logger = Logger();
        captureFb = createWithImplementation<IFunctionBlock, modules::asam_cmp_data_sink_module::CaptureFb>(
            moduleInfo, Context(Scheduler(logger), logger, TypeManager(), nullptr), nullptr, "capture_module_0", publisher, capturePacketsPublisher);

        captureFb.getPropertyValue("AddInterface").execute();
        interfaceFb = captureFb.getFunctionBlocks().getItemAt(0);
    }

protected:
    ModuleInfoPtr moduleInfo{};
    FunctionBlockPtr captureFb;
    FunctionBlockPtr interfaceFb;
    modules::asam_cmp_data_sink_module::DataPacketsPublisher publisher;
    modules::asam_cmp_data_sink_module::CapturePacketsPublisher capturePacketsPublisher;
};

TEST_F(InterfaceFbTest, NotNull)
{
    ASSERT_NE(interfaceFb, nullptr);
}

TEST_F(InterfaceFbTest, FunctionBlockType)
{
    auto type = interfaceFb.getFunctionBlockType();
    ASSERT_EQ(type.getId(), "AsamCmpInterface");
    ASSERT_EQ(type.getName(), "AsamCmpInterface");
    ASSERT_EQ(type.getDescription(), "ASAM CMP Interface");
}

TEST_F(InterfaceFbTest, AvailableFunctionBlockTypes)
{
    auto availableTypes = interfaceFb.getAvailableFunctionBlockTypes();
    ASSERT_EQ(availableTypes.getCount(), 1u);
    ASSERT_TRUE(availableTypes.hasKey("AsamCmpStream"));
    ASSERT_EQ(
        availableTypes.get("AsamCmpStream"),
        modules::asam_cmp_data_sink_module::StreamFb::CreateType(interfaceFb.getFunctionBlockType().getModuleInfo())
    );
}

TEST_F(InterfaceFbTest, OnAddFunctionBlocks)
{
    EXPECT_NO_THROW(interfaceFb.addFunctionBlock("AsamCmpStream"));
    EXPECT_THROW(interfaceFb.addFunctionBlock("AsamCmpCapture"), daq::NotFoundException);
}

TEST_F(InterfaceFbTest, CaptureModuleProperties)
{
    ASSERT_TRUE(interfaceFb.hasProperty("InterfaceId"));
    ASSERT_TRUE(interfaceFb.hasProperty("PayloadType"));
    ASSERT_TRUE(interfaceFb.hasProperty("AddStream"));
    ASSERT_TRUE(interfaceFb.hasProperty("RemoveStream"));
}

TEST_F(InterfaceFbTest, InterfaceId)
{
    captureFb.getPropertyValue("AddInterface").execute();
    ASSERT_EQ(captureFb.getFunctionBlocks().getCount(), 2u);

    const auto interfaceFb2 = captureFb.getFunctionBlocks().getItemAt(1);

    uint32_t id1 = interfaceFb.getPropertyValue("InterfaceId");
    uint32_t id2 = interfaceFb2.getPropertyValue("InterfaceId");
    ASSERT_NE(id1, id2);

    interfaceFb2.setPropertyValue("InterfaceId", id1);
    id1 = interfaceFb.getPropertyValue("InterfaceId");
    id2 = interfaceFb2.getPropertyValue("InterfaceId");
    ASSERT_EQ(id1, id2);

    interfaceFb2.setPropertyValue("InterfaceId", id2 + 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_EQ(interfaceFb2.getPropertyValue("InterfaceId"), id2 + 1);
}

TEST_F(InterfaceFbTest, InterfaceIdMin)
{
    constexpr auto minValue = static_cast<Int>(std::numeric_limits<uint32_t>::min());
    interfaceFb.setPropertyValue("InterfaceId", minValue - 1);
    ASSERT_EQ(static_cast<Int>(interfaceFb.getPropertyValue("InterfaceId")), minValue);
}

TEST_F(InterfaceFbTest, InterfaceIdMax)
{
    constexpr auto newMax = static_cast<Int>(std::numeric_limits<uint32_t>::max()) + 1;
    constexpr auto maxMax = static_cast<Int>(std::numeric_limits<uint32_t>::max());
    interfaceFb.setPropertyValue("InterfaceId", newMax);
    ASSERT_EQ(static_cast<Int>(interfaceFb.getPropertyValue("InterfaceId")), maxMax);
}

TEST_F(InterfaceFbTest, AddRemoveStream)
{
    ProcedurePtr addStream = interfaceFb.getPropertyValue("AddStream");
    interfaceFb.setPropertyValue("PayloadType", 1);
    addStream();
    addStream();

    auto s1 = interfaceFb.getFunctionBlocks().getItemAt(0), s2 = interfaceFb.getFunctionBlocks().getItemAt(1);

    ASSERT_NE(s1.getPropertyValue("StreamId"), s2.getPropertyValue("StreamId"));

    interfaceFb.getPropertyValue("RemoveStream").execute(0);

    ASSERT_EQ(interfaceFb.getFunctionBlocks().getCount(), 1u);
}

TEST_F(InterfaceFbTest, TestBeginUpdateEndUpdate)
{
    const size_t oldInterfaceId = interfaceFb.getPropertyValue("InterfaceId");
    const size_t oldPayloadType = interfaceFb.getPropertyValue("PayloadType");

    const size_t interfaceId = (oldInterfaceId == 1 ? 2 : 1);
    const size_t payloadType = (oldPayloadType == 0 ? 1 : 0);

    const ProcedurePtr createProc = interfaceFb.getPropertyValue("AddStream");
    const ProcedurePtr removeProc = interfaceFb.getPropertyValue("RemoveStream");

    interfaceFb.beginUpdate();
    interfaceFb.setPropertyValue("InterfaceId", interfaceId);
    interfaceFb.setPropertyValue("PayloadType", payloadType);

    ASSERT_EQ(interfaceFb.getPropertyValue("InterfaceId"), oldInterfaceId);
    ASSERT_EQ(interfaceFb.getPropertyValue("PayloadType"), oldPayloadType);
    ASSERT_ANY_THROW(createProc());
    ASSERT_ANY_THROW(removeProc(0));
    interfaceFb.endUpdate();

    ASSERT_EQ(interfaceFb.getPropertyValue("InterfaceId"), interfaceId);
    ASSERT_EQ(interfaceFb.getPropertyValue("PayloadType"), payloadType);
    ASSERT_NO_THROW(createProc());
    ASSERT_NO_THROW(removeProc(0));
}

TEST_F(InterfaceFbTest, TestBeginUpdateEndUpdateWithWrongId)
{
    captureFb.getPropertyValue("AddInterface").execute();

    const auto interfaceFb2 = captureFb.getFunctionBlocks().getItemAt(1);
    const size_t id2 = interfaceFb2.getPropertyValue("InterfaceId");

    const size_t oldInterfaceId = interfaceFb.getPropertyValue("InterfaceId");
    const size_t oldPayloadType = interfaceFb.getPropertyValue("PayloadType");

    const size_t payloadType = (oldPayloadType == 0 ? 1 : 0);

    interfaceFb.beginUpdate();
    interfaceFb.setPropertyValue("InterfaceId", id2);
    interfaceFb.setPropertyValue("PayloadType", payloadType);

    ASSERT_EQ(interfaceFb.getPropertyValue("InterfaceId"), oldInterfaceId);
    ASSERT_EQ(interfaceFb.getPropertyValue("PayloadType"), oldPayloadType);
    interfaceFb.endUpdate();

    ASSERT_EQ(interfaceFb.getPropertyValue("InterfaceId"), id2);
    ASSERT_EQ(interfaceFb.getPropertyValue("PayloadType"), payloadType);
}
