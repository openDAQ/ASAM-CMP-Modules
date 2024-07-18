#include <coretypes/common.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/signal_factory.h>
#include <asam_cmp_capture_module/module_dll.h>
#include <asam_cmp_capture_module/version.h>
#include <thread>
#include <gtest/gtest.h>

using namespace daq;

class AsamCmpCaptureModuleTest : public testing::Test
{
protected:
    AsamCmpCaptureModuleTest()
    {
        logger = Logger();
        createModule(&module, Context(Scheduler(logger), logger, nullptr, nullptr, nullptr));
    }

    ModulePtr getModule()
    {
        return module;
    }

private:
    LoggerPtr logger;
    ModulePtr module;

};

TEST_F(AsamCmpCaptureModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(AsamCmpCaptureModuleTest, ModuleName)
{
    auto module = getModule();
    ASSERT_EQ(module.getName(), "ASAM CMP CaptureModule");
}

TEST_F(AsamCmpCaptureModuleTest, VersionAvailable)
{
    auto module = getModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(AsamCmpCaptureModuleTest, VersionCorrect)
{
    auto module = getModule();
    auto version = module.getVersionInfo();

    ASSERT_EQ(version.getMajor(), ASAM_CMP_CAPTURE_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), ASAM_CMP_CAPTURE_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), ASAM_CMP_CAPTURE_MODULE_PATCH_VERSION);
}

TEST_F(AsamCmpCaptureModuleTest, EnumerateDevices)
{
    auto module = getModule();

    ListPtr<IDeviceInfo> deviceInfoDict;
    ASSERT_NO_THROW(deviceInfoDict = module.getAvailableDevices());
    ASSERT_EQ(deviceInfoDict.getCount(), 0u);
}

TEST_F(AsamCmpCaptureModuleTest, AcceptsConnectionStringNull)
{
    auto module = getModule();
    ASSERT_THROW(module.acceptsConnectionParameters(nullptr), ArgumentNullException);
}

TEST_F(AsamCmpCaptureModuleTest, AcceptsConnectionStringEmpty)
{
    auto module = getModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters(""));
    ASSERT_FALSE(accepts);
}

TEST_F(AsamCmpCaptureModuleTest, AcceptsConnectionStringInvalid)
{
    auto module = getModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters("drfrfgt"));
    ASSERT_FALSE(accepts);
}

 TEST_F(AsamCmpCaptureModuleTest, GetAvailableComponentTypes)
 {
     const auto module = getModule();

     DictPtr<IString, IDeviceType> deviceTypes;
     ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
     ASSERT_EQ(deviceTypes.getCount(), 0u);

     DictPtr<IString, IServerType> serverTypes;
     ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
     ASSERT_EQ(serverTypes.getCount(), 0u);

     DictPtr<IString, IFunctionBlockType> functionBlockTypes;
     ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
     ASSERT_TRUE(functionBlockTypes.assigned());
     ASSERT_EQ(functionBlockTypes.getCount(), 1u);

     ASSERT_TRUE(functionBlockTypes.hasKey("asam_cmp_capture"));
     ASSERT_EQ("asam_cmp_capture", functionBlockTypes.get("asam_cmp_capture").getId());
 }

TEST_F(AsamCmpCaptureModuleTest, CreateFunctionBlockNotFound)
{
    const auto module = getModule();

    ASSERT_THROW(module.createFunctionBlock("test", nullptr, "id"), NotFoundException);
}

TEST_F(AsamCmpCaptureModuleTest, CreateFunctionBlockCaptureModule)
{
    const auto module = getModule();

    auto fb = module.createFunctionBlock("asam_cmp_capture", nullptr, "id");
    ASSERT_TRUE(fb.assigned());
}
