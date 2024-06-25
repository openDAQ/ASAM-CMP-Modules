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

using AsamCmpCapturModuleTest = testing::Test;
using namespace daq;

static ModulePtr CreateModule()
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, Context(Scheduler(logger), logger, nullptr, nullptr, nullptr));
    return module;
}

TEST_F(AsamCmpCapturModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(AsamCmpCapturModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getName(), "ASAM CMP CaptureModule");
}

TEST_F(AsamCmpCapturModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(AsamCmpCapturModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getVersionInfo();

    ASSERT_EQ(version.getMajor(), ASAM_CMP_CAPTURE_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), ASAM_CMP_CAPTURE_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), ASAM_CMP_CAPTURE_MODULE_PATCH_VERSION);
}

TEST_F(AsamCmpCapturModuleTest, EnumerateDevices)
{
    auto module = CreateModule();

    ListPtr<IDeviceInfo> deviceInfoDict;
    ASSERT_NO_THROW(deviceInfoDict = module.getAvailableDevices());
    ASSERT_EQ(deviceInfoDict.getCount(), 0u);
}

TEST_F(AsamCmpCapturModuleTest, AcceptsConnectionStringNull)
{
    auto module = CreateModule();
    ASSERT_THROW(module.acceptsConnectionParameters(nullptr), ArgumentNullException);
}

TEST_F(AsamCmpCapturModuleTest, AcceptsConnectionStringEmpty)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters(""));
    ASSERT_FALSE(accepts);
}

TEST_F(AsamCmpCapturModuleTest, AcceptsConnectionStringInvalid)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters("drfrfgt"));
    ASSERT_FALSE(accepts);
}

//TODO: reserved as reference once function blocks are implemented
// TEST_F(AsamCmpCapturModuleTest, GetAvailableComponentTypes)
// {
//     const auto module = CreateModule();

//     DictPtr<IString, IDeviceType> deviceTypes;
//     ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
//     ASSERT_EQ(deviceTypes.getCount(), 0u);

//     DictPtr<IString, IServerType> serverTypes;
//     ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
//     ASSERT_EQ(serverTypes.getCount(), 0u);

//     DictPtr<IString, IFunctionBlockType> functionBlockTypes;
//     ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
//     ASSERT_TRUE(functionBlockTypes.assigned());
//     ASSERT_EQ(functionBlockTypes.getCount(), 7u);

//     ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_renderer"));
//     ASSERT_EQ("ref_fb_module_renderer", functionBlockTypes.get("ref_fb_module_renderer").getId());

//     ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_statistics"));
//     ASSERT_EQ("ref_fb_module_statistics", functionBlockTypes.get("ref_fb_module_statistics").getId());

//     ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_power"));
//     ASSERT_EQ("ref_fb_module_power", functionBlockTypes.get("ref_fb_module_power").getId());

//     ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_scaling"));
//     ASSERT_EQ("ref_fb_module_scaling", functionBlockTypes.get("ref_fb_module_scaling").getId());

//     ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_classifier"));
//     ASSERT_EQ("ref_fb_module_classifier", functionBlockTypes.get("ref_fb_module_classifier").getId());

//     ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_trigger"));
//     ASSERT_EQ("ref_fb_module_trigger", functionBlockTypes.get("ref_fb_module_trigger").getId());
// }

TEST_F(AsamCmpCapturModuleTest, CreateFunctionBlockNotFound)
{
    const auto module = CreateModule();

    ASSERT_THROW(module.createFunctionBlock("test", nullptr, "id"), NotFoundException);
}

// TODO: reserved as reference once function blocks are implemented
// TEST_F(AsamCmpCapturModuleTest, CreateFunctionBlockStatistics)
// {
//     const auto module = CreateModule();
