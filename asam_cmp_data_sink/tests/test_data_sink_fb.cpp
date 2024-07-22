#include <asam_cmp_data_sink/status_handler.h>
#include <asam_cmp_data_sink/module_dll.h>

#include <asam_cmp/capture_module_payload.h>
#include <asam_cmp/interface_payload.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>

using namespace daq;
using namespace daq::modules::asam_cmp_data_sink_module;
using ASAM::CMP::CaptureModulePayload;
using ASAM::CMP::InterfacePayload;
using ASAM::CMP::Packet;

class DataSinkFbFixture : public ::testing::Test
{
protected:
    DataSinkFbFixture()
    {
        auto logger = Logger();
        createModule(&module, Context(Scheduler(logger), logger, TypeManager(), nullptr));
        dataSinkModuleFb = module.createFunctionBlock("asam_cmp_data_sink_module", nullptr, "id");
        funcBlock = dataSinkModuleFb.getFunctionBlocks(search::Recursive(search::LocalId("asam_cmp_data_sink")))[0];
        auto statusFb = dataSinkModuleFb.getFunctionBlocks(search::Recursive(search::LocalId("asam_cmp_status")))[0];
        statusHandler = statusFb.asPtrOrNull<IStatusHandler>();

        CaptureModulePayload cmPayload;
        cmPayload.setData(deviceDescr, "", "", "", {});
        cmPacket = std::make_shared<Packet>();
        cmPacket->setVersion(1);
        cmPacket->setPayload(cmPayload);

        InterfacePayload ifPayload;
        std::vector<uint8_t> streams = {1};
        ifPayload.setInterfaceId(0);
        ifPayload.setData(streams.data(), static_cast<uint16_t>(streams.size()), nullptr, 0);
        ifPacket = std::make_shared<Packet>();
        ifPacket->setVersion(1);
        ifPacket->setPayload(ifPayload);
    }

protected:
    static constexpr std::string_view deviceDescr = "Device Description";

protected:
    ModulePtr module;
    FunctionBlockPtr funcBlock;
    FunctionBlockPtr dataSinkModuleFb;
    IStatusHandler* statusHandler;

    std::shared_ptr<Packet> cmPacket;
    std::shared_ptr<Packet> ifPacket;
};

TEST_F(DataSinkFbFixture, NotNull)
{
    ASSERT_NE(module, nullptr);
    ASSERT_NE(funcBlock, nullptr);
    ASSERT_NE(statusHandler, nullptr);
}

TEST_F(DataSinkFbFixture, FunctionBlockType)
{
    auto type = funcBlock.getFunctionBlockType();
    ASSERT_EQ(type.getId(), "asam_cmp_data_sink");
    ASSERT_EQ(type.getName(), "AsamCmpDataSink");
    ASSERT_EQ(type.getDescription(), "ASAM CMP Data Sink");
}

TEST_F(DataSinkFbFixture, AddCaptureModuleFromStatus)
{
    auto proc = daq::Procedure([]() {});
    EXPECT_THROW(funcBlock.setPropertyValue("AddCaptureModuleFromStatus", proc), daq::AccessDeniedException);

    statusHandler->processStatusPacket(cmPacket);
    statusHandler->processStatusPacket(ifPacket);

    ProcedurePtr addCaptureModuleFromStatus = funcBlock.getPropertyValue("AddCaptureModuleFromStatus");
    addCaptureModuleFromStatus(0);
    addCaptureModuleFromStatus(0);
    ASSERT_EQ(funcBlock.getFunctionBlocks().getCount(), 2);
    auto captureModule = funcBlock.getFunctionBlocks()[0];
    ASSERT_EQ(captureModule.getFunctionBlocks(search::LocalId("asam_cmp_interface_0")).getCount(), 1);
    auto interfaceFb = captureModule.getFunctionBlocks(search::LocalId("asam_cmp_interface_0"))[0];
    ASSERT_EQ(interfaceFb.getFunctionBlocks(search::LocalId("asam_cmp_stream_1")).getCount(), 1);

    cmPacket->setDeviceId(3);
    statusHandler->processStatusPacket(cmPacket);

    ifPacket->setDeviceId(3);
    auto& ifPayload = static_cast<InterfacePayload&>(ifPacket->getPayload());
    ifPayload.setInterfaceId(22);
    statusHandler->processStatusPacket(ifPacket);

    ifPayload.setInterfaceId(33);
    std::vector<uint8_t> streams = {3, 5, 10};
    ifPayload.setData(streams.data(), static_cast<uint16_t>(streams.size()), nullptr, 0);
    statusHandler->processStatusPacket(ifPacket);

    addCaptureModuleFromStatus(1);
    ASSERT_EQ(funcBlock.getFunctionBlocks().getCount(), 3);
    captureModule = funcBlock.getFunctionBlocks()[2];
    ASSERT_EQ(captureModule.getFunctionBlocks(search::LocalId("asam_cmp_interface_22")).getCount(), 1);
    interfaceFb = captureModule.getFunctionBlocks(search::LocalId("asam_cmp_interface_22"))[0];
    ASSERT_EQ(interfaceFb.getFunctionBlocks(search::LocalId("asam_cmp_stream_1")).getCount(), 1);
    ASSERT_EQ(captureModule.getFunctionBlocks(search::LocalId("asam_cmp_interface_33")).getCount(), 1);
    interfaceFb = captureModule.getFunctionBlocks(search::LocalId("asam_cmp_interface_33"))[0];
    ASSERT_EQ(interfaceFb.getFunctionBlocks(search::LocalId("asam_cmp_stream_3")).getCount(), 1);
    ASSERT_EQ(interfaceFb.getFunctionBlocks(search::LocalId("asam_cmp_stream_5")).getCount(), 1);
    ASSERT_EQ(interfaceFb.getFunctionBlocks(search::LocalId("asam_cmp_stream_10")).getCount(), 1);
}

TEST_F(DataSinkFbFixture, AddCaptureModuleEmpty)
{
    auto proc = daq::Procedure([]() {});
    EXPECT_THROW(funcBlock.setPropertyValue("AddCaptureModuleEmpty", proc), daq::AccessDeniedException);

    ProcedurePtr func = funcBlock.getPropertyValue("AddCaptureModuleEmpty");
    func();
    func();
    ASSERT_EQ(funcBlock.getFunctionBlocks().getCount(), 2);
}

TEST_F(DataSinkFbFixture, RemoveCaptureModule)
{
    auto proc = daq::Procedure([]() {});
    EXPECT_THROW(funcBlock.setPropertyValue("RemoveCaptureModule", proc), daq::AccessDeniedException);

    ASSERT_EQ(funcBlock.getFunctionBlocks().getCount(), 0);
    ProcedurePtr addFunc = funcBlock.getPropertyValue("AddCaptureModuleEmpty");
    addFunc();
    ProcedurePtr removeFunc = funcBlock.getPropertyValue("RemoveCaptureModule");
    removeFunc(0);
    ASSERT_EQ(funcBlock.getFunctionBlocks().getCount(), 0);
}
