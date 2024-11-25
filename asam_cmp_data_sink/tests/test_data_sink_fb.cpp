#include <asam_cmp_data_sink/data_sink_fb.h>
#include <asam_cmp_data_sink/status_fb_impl.h>
#include <asam_cmp_data_sink/status_handler.h>
#include <asam_cmp_data_sink/capture_fb.h>

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

class DataSinkFbTest : public ::testing::Test
{
protected:
    DataSinkFbTest()
    {
        auto logger = Logger();
        context = Context(Scheduler(logger), logger, TypeManager(), nullptr);
        statusFb =
            createWithImplementation<IFunctionBlock, modules::asam_cmp_data_sink_module::StatusFbImpl>(context, nullptr, "asam_cmp_status");
        statusHandler = statusFb.asPtrOrNull<IStatusHandler>();

        funcBlock = createWithImplementation<IFunctionBlock, modules::asam_cmp_data_sink_module::DataSinkFb>(
            context, nullptr, "asam_cmp_data_sink", statusHandler->getStatusMt(), publisher, capturePacketPublisher);

        CaptureModulePayload cmPayload;
        std::vector<uint8_t> vendorData = std::vector<uint8_t>(begin(vendorDataAsString), end(vendorDataAsString));
        cmPayload.setData(deviceDescr, serialNumber, hardwareVersion, softwareVersion, vendorData);
        cmPacket = std::make_shared<Packet>();
        cmPacket->setVersion(1);
        cmPacket->setPayload(cmPayload);

        constexpr uint8_t canInterfaceType = 1;

        InterfacePayload ifPayload;
        std::vector<uint8_t> streams = {1};
        ifPayload.setInterfaceId(0);
        ifPayload.setInterfaceType(canInterfaceType);
        ifPayload.setData(streams.data(), static_cast<uint16_t>(streams.size()), nullptr, 0);
        ifPacket = std::make_shared<Packet>();
        ifPacket->setVersion(1);
        ifPacket->setPayload(ifPayload);
    }

protected:
    static constexpr std::string_view deviceDescr = "Device Description";
    static constexpr std::string_view serialNumber = "Serial Number";
    static constexpr std::string_view hardwareVersion = "Hardware Version";
    static constexpr std::string_view softwareVersion = "Software Version";
    static constexpr std::string_view vendorDataAsString = "Vendor Data";

protected:
    modules::asam_cmp_data_sink_module::DataPacketsPublisher publisher;
    modules::asam_cmp_data_sink_module::CapturePacketsPublisher capturePacketPublisher;
    ContextPtr context;
    FunctionBlockPtr funcBlock;
    FunctionBlockPtr statusFb;
    IStatusHandler* statusHandler;

    std::shared_ptr<Packet> cmPacket;
    std::shared_ptr<Packet> ifPacket;
};

TEST_F(DataSinkFbTest, NotNull)
{
    ASSERT_NE(funcBlock, nullptr);
    ASSERT_NE(statusHandler, nullptr);
}

TEST_F(DataSinkFbTest, FunctionBlockType)
{
    auto type = funcBlock.getFunctionBlockType();
    ASSERT_EQ(type.getId(), "AsamCmpDataSink");
    ASSERT_EQ(type.getName(), "AsamCmpDataSink");
    ASSERT_EQ(type.getDescription(), "ASAM CMP Data Sink");
}

TEST_F(DataSinkFbTest, AvailableFunctionBlockTypes)
{
    auto availableTypes = funcBlock.getAvailableFunctionBlockTypes();
    ASSERT_EQ(availableTypes.getCount(), 1u);
    ASSERT_TRUE(availableTypes.hasKey("AsamCmpCapture"));
    ASSERT_EQ(availableTypes.get("AsamCmpCapture"), CaptureFb::CreateType());
}

TEST_F(DataSinkFbTest, OnAddFunctionBlocks)
{
    EXPECT_NO_THROW(funcBlock.addFunctionBlock("AsamCmpCapture"));
    EXPECT_THROW(funcBlock.addFunctionBlock("AsamCmpStream"), daq::NotFoundException);
}

TEST_F(DataSinkFbTest, AddCaptureModuleFromStatusReadOnly)
{
    auto proc = daq::Procedure([]() {});
    EXPECT_THROW(funcBlock.setPropertyValue("AddCaptureModuleFromStatus", proc), daq::AccessDeniedException);
}

TEST_F(DataSinkFbTest, AddCaptureModuleFromStatus)
{
    statusHandler->processStatusPacket(cmPacket);
    statusHandler->processStatusPacket(ifPacket);

    ProcedurePtr addCaptureModuleFromStatus = funcBlock.getPropertyValue("AddCaptureModuleFromStatus");
    addCaptureModuleFromStatus(0);
    addCaptureModuleFromStatus(0);
    ASSERT_EQ(funcBlock.getFunctionBlocks().getCount(), 2u);
    auto captureModule = funcBlock.getFunctionBlocks()[0];

    StringPtr propVal = captureModule.getPropertyValue("DeviceDescription");
    ASSERT_EQ(propVal.toStdString(), deviceDescr);
    propVal = captureModule.getPropertyValue("SerialNumber");
    ASSERT_EQ(propVal.toStdString(), serialNumber);
    propVal = captureModule.getPropertyValue("HardwareVersion");
    ASSERT_EQ(propVal.toStdString(), hardwareVersion);
    propVal = captureModule.getPropertyValue("SoftwareVersion");
    ASSERT_EQ(propVal.toStdString(), softwareVersion);
    propVal = captureModule.getPropertyValue("VendorData");
    ASSERT_EQ(propVal.toStdString(), vendorDataAsString);

    int targetInterfaceId = 0;
    FunctionPtr isCorrectInterface =
        Function([&targetInterfaceId](FunctionBlockPtr arg)
                 { return arg.hasProperty("InterfaceId") && (arg.getPropertyValue("InterfaceId") == targetInterfaceId); });
    SearchFilterPtr interfaceFilter = search::Custom(isCorrectInterface);

    ASSERT_EQ(captureModule.getFunctionBlocks(interfaceFilter).getCount(), 1u);
    auto interfaceFb = captureModule.getFunctionBlocks(interfaceFilter)[0];

    int targetStreamId = 1;
    FunctionPtr isCorrectStream = Function([&targetStreamId](FunctionBlockPtr arg)
                                           { return arg.hasProperty("StreamId") && (arg.getPropertyValue("StreamId") == targetStreamId); });
    SearchFilterPtr streamFilter = search::Custom(isCorrectStream);

    ASSERT_EQ(interfaceFb.getFunctionBlocks(streamFilter).getCount(), 1u);

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
    ASSERT_EQ(funcBlock.getFunctionBlocks().getCount(), 3u);
    captureModule = funcBlock.getFunctionBlocks()[2];

    targetInterfaceId = 22;
    ASSERT_EQ(captureModule.getFunctionBlocks(interfaceFilter).getCount(), 1u);
    interfaceFb = captureModule.getFunctionBlocks(interfaceFilter)[0];

    targetStreamId = 1;
    ASSERT_EQ(interfaceFb.getFunctionBlocks(streamFilter).getCount(), 1u);

    targetInterfaceId = 33;
    ASSERT_EQ(captureModule.getFunctionBlocks(interfaceFilter).getCount(), 1u);
    interfaceFb = captureModule.getFunctionBlocks(interfaceFilter)[0];

    targetStreamId = 3;
    ASSERT_EQ(interfaceFb.getFunctionBlocks(streamFilter).getCount(), 1u);
    targetStreamId = 5;
    ASSERT_EQ(interfaceFb.getFunctionBlocks(streamFilter).getCount(), 1u);
    targetStreamId = 10;
    ASSERT_EQ(interfaceFb.getFunctionBlocks(streamFilter).getCount(), 1u);
}

TEST_F(DataSinkFbTest, RemoveCaptureFromStatus)
{
    static const uint16_t deviceId = 1;
    cmPacket->setDeviceId(deviceId);
    statusHandler->processStatusPacket(cmPacket);

    funcBlock.getPropertyValue("AddCaptureModuleFromStatus").execute(0);
    capturePacketPublisher.publish(deviceId, cmPacket);

    funcBlock.getPropertyValue("RemoveCaptureModule").execute(0);
    ASSERT_NO_THROW(capturePacketPublisher.publish(deviceId, cmPacket));
}

TEST_F(DataSinkFbTest, AddCaptureModuleEmptyReadOnly)
{
    auto proc = daq::Procedure([]() {});
    EXPECT_THROW(funcBlock.setPropertyValue("AddCaptureModuleEmpty", proc), daq::AccessDeniedException);
}

TEST_F(DataSinkFbTest, AddCaptureModuleEmpty)
{
    ProcedurePtr func = funcBlock.getPropertyValue("AddCaptureModuleEmpty");
    func();
    func();
    ASSERT_EQ(funcBlock.getFunctionBlocks().getCount(), 2u);
}

TEST_F(DataSinkFbTest, UpdateDeviceInfo)
{
    funcBlock.getPropertyValue("AddCaptureModuleEmpty").execute();
    auto captureModule = funcBlock.getFunctionBlocks()[0];
    capturePacketPublisher.publish(cmPacket->getDeviceId(), cmPacket);

    StringPtr propVal = captureModule.getPropertyValue("DeviceDescription");
    ASSERT_EQ(propVal.toStdString(), deviceDescr);
    propVal = captureModule.getPropertyValue("SerialNumber");
    ASSERT_EQ(propVal.toStdString(), serialNumber);
    propVal = captureModule.getPropertyValue("HardwareVersion");
    ASSERT_EQ(propVal.toStdString(), hardwareVersion);
    propVal = captureModule.getPropertyValue("SoftwareVersion");
    ASSERT_EQ(propVal.toStdString(), softwareVersion);
    propVal = captureModule.getPropertyValue("VendorData");
    ASSERT_EQ(propVal.toStdString(), vendorDataAsString);
}

TEST_F(DataSinkFbTest, RemoveCaptureModuleReadOnly)
{
    auto proc = daq::Procedure([]() {});
    EXPECT_THROW(funcBlock.setPropertyValue("RemoveCaptureModule", proc), daq::AccessDeniedException);
}

TEST_F(DataSinkFbTest, RemoveCaptureModule)
{
    ASSERT_EQ(funcBlock.getFunctionBlocks().getCount(), 0u);
    ProcedurePtr addFunc = funcBlock.getPropertyValue("AddCaptureModuleEmpty");
    addFunc();
    auto captureFb = funcBlock.getFunctionBlocks().getItemAt(0);
    captureFb.getPropertyValue("AddInterface").execute();
    auto interfaceFb = captureFb.getFunctionBlocks().getItemAt(0);
    interfaceFb.getPropertyValue("AddStream").execute();

    ProcedurePtr removeFunc = funcBlock.getPropertyValue("RemoveCaptureModule");
    removeFunc(0);
    ASSERT_EQ(funcBlock.getFunctionBlocks().getCount(), 0u);
    ASSERT_EQ(publisher.size(), 0u);
}

TEST_F(DataSinkFbTest, DefaultDevicesIds)
{
    funcBlock.getPropertyValue("AddCaptureModuleEmpty").execute();
    funcBlock.getPropertyValue("AddCaptureModuleEmpty").execute();

    const auto captureFb1 = funcBlock.getFunctionBlocks()[0];
    const auto captureFb2 = funcBlock.getFunctionBlocks()[1];
    const Int id1 = captureFb1.getPropertyValue("DeviceId");
    const Int id2 = captureFb2.getPropertyValue("DeviceId");
    ASSERT_EQ(id1, id2);
}
