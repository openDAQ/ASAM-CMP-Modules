#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/module_info_factory.h>
#include <gtest/gtest.h>

#include <asam_cmp_common_lib/ethernet_pcpp_mock.h>

#include <asam_cmp_capture_module/capture_module.h>
#include <asam_cmp_capture_module/capture_module_fb.h>
#include <asam_cmp_capture_module/version.h>


using namespace daq;
using namespace testing;
using namespace daq::modules::asam_cmp_capture_module;

class CaptureModuleFbTest : public::testing::Test
{
protected:
    CaptureModuleFbTest()
        : logger(Logger())
        , context(Context(Scheduler(logger), logger, nullptr, nullptr))
        , moduleInfo(ModuleInfo(VersionInfo(1,2,3), "", ""))
        , names(List<IString>())
        , descriptions(List<IString>())
        , ethernetWrapper(std::make_shared<asam_cmp_common_lib::EthernetPcppMock>())
    {
        names.pushBack("DummyName");
        descriptions.pushBack("DummyDescription");
        auto startStub = []() {};
        auto stopStub = []() {};

        EXPECT_CALL(*ethernetWrapper, setDevice(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*ethernetWrapper, startCapture(_)).WillRepeatedly(startStub);
        EXPECT_CALL(*ethernetWrapper, stopCapture()).WillRepeatedly(stopStub);
        EXPECT_CALL(*ethernetWrapper, getEthernetDevicesNamesList()).WillRepeatedly(Return(names));
        EXPECT_CALL(*ethernetWrapper, getEthernetDevicesDescriptionsList()).WillRepeatedly(Return(descriptions));
        EXPECT_CALL(*ethernetWrapper, sendPacket(_))
            .WillRepeatedly(WithArgs<0>(Invoke(
                [&](const std::vector<uint8_t>& data) { }
        )));

        captureModuleFb = createWithImplementation<IFunctionBlock, CaptureModuleFb>(moduleInfo, context, nullptr, "dummy_id", ethernetWrapper);
    }

protected:
    LoggerPtr logger;
    ContextPtr context;
    ModuleInfoPtr moduleInfo;

    ListPtr<StringPtr> names;
    ListPtr<StringPtr> descriptions;
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppMock> ethernetWrapper;

    FunctionBlockPtr captureModuleFb;
};

TEST_F(CaptureModuleFbTest, VersionScaling)
{
    ASSERT_EQ(captureModuleFb.getFunctionBlockType().getModuleInfo().getVersionInfo(), moduleInfo.getVersionInfo());

    auto captureFb = captureModuleFb.getFunctionBlocks().getItemAt(0);
    ASSERT_EQ(captureFb.getFunctionBlockType().getModuleInfo().getVersionInfo(), moduleInfo.getVersionInfo());

    auto itfFB = captureFb.addFunctionBlock("AsamCmpInterface");
    ASSERT_EQ(itfFB.getFunctionBlockType().getModuleInfo().getVersionInfo(), moduleInfo.getVersionInfo());

    auto streamFb = itfFB.addFunctionBlock("AsamCmpStream");
    ASSERT_EQ(itfFB.getFunctionBlockType().getModuleInfo().getVersionInfo(), moduleInfo.getVersionInfo());
}

