#include <asam_cmp_data_sink/module_dll.h>
#include <asam_cmp_data_sink/version.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>

using DataSinkModuleTest = testing::Test;
using namespace daq;

static ModulePtr CreateModule()
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, Context(Scheduler(logger), logger, nullptr, nullptr));
    return module;
}

TEST_F(DataSinkModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(DataSinkModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getName(), "ASAM CMP Data Sink Module");
}

TEST_F(DataSinkModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(DataSinkModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getVersionInfo();

    ASSERT_EQ(version.getMajor(), ASAM_CMP_DATA_SINK_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), ASAM_CMP_DATA_SINK_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), ASAM_CMP_DATA_SINK_PATCH_VERSION);
}

TEST_F(DataSinkModuleTest, EnumerateDevices)
{
    auto module = CreateModule();

    ListPtr<IDeviceInfo> deviceInfoDict;
    ASSERT_NO_THROW(deviceInfoDict = module.getAvailableDevices());
    ASSERT_EQ(deviceInfoDict.getCount(), 0u);
}

TEST_F(DataSinkModuleTest, AcceptsConnectionStringNull)
{
    auto module = CreateModule();
    ASSERT_THROW(module.acceptsConnectionParameters(nullptr), ArgumentNullException);
}

TEST_F(DataSinkModuleTest, AcceptsConnectionStringEmpty)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters(""));
    ASSERT_FALSE(accepts);
}

TEST_F(DataSinkModuleTest, AcceptsConnectionStringInvalid)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters("drfrfgt"));
    ASSERT_FALSE(accepts);
}

TEST_F(DataSinkModuleTest, GetAvailableComponentTypes)
{
    const auto module = CreateModule();

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

    ASSERT_TRUE(functionBlockTypes.hasKey("asam_cmp_data_sink_module"));
    ASSERT_EQ("asam_cmp_data_sink_module", functionBlockTypes.get("asam_cmp_data_sink_module").getId());
}

TEST_F(DataSinkModuleTest, CreateFunctionBlockNotFound)
{
    const auto module = CreateModule();

    ASSERT_THROW(module.createFunctionBlock("test", nullptr, "id"), NotFoundException);
}

TEST_F(DataSinkModuleTest, CreateAsamCmpDataSinkModuleFunctionBlock)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("asam_cmp_data_sink_module", nullptr, "id");
    ASSERT_TRUE(fb.assigned());
}
