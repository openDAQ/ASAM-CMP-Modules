#include <asam_cmp_data_sink/module_dll.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>

using namespace daq;

class AsamCmpDataSinkModuleFbFixture : public ::testing::Test
{
protected:
    AsamCmpDataSinkModuleFbFixture()
    {
        auto logger = Logger();
        createModule(&module, Context(Scheduler(logger), logger, TypeManager(), nullptr));
        funcBlock = module.createFunctionBlock("asam_cmp_data_sink_module", nullptr, "id");
    }

protected:
    template <typename T>
    void testProperty(const StringPtr& name, T newValue, bool success = true);

protected:
    ModulePtr module;
    FunctionBlockPtr funcBlock;
};

TEST_F(AsamCmpDataSinkModuleFbFixture, NotNull)
{
    ASSERT_NE(module, nullptr);
    ASSERT_NE(funcBlock, nullptr);
}

TEST_F(AsamCmpDataSinkModuleFbFixture, FunctionBlockType)
{
    auto type = funcBlock.getFunctionBlockType();
    ASSERT_EQ(type.getId(), "asam_cmp_data_sink_module");
    ASSERT_EQ(type.getName(), "AsamCmpDataSinkModule");
    ASSERT_EQ(type.getDescription(), "ASAM CMP Data Sink Module");
}

template <typename T>
void AsamCmpDataSinkModuleFbFixture::testProperty(const StringPtr& name, T newValue, bool success)
{
    funcBlock.setPropertyValue(name, newValue);
    const T value = funcBlock.getPropertyValue(name);
    if (success)
        ASSERT_EQ(value, newValue);
    else
        ASSERT_NE(value, newValue);
}

TEST_F(AsamCmpDataSinkModuleFbFixture, NetworkAdaptersProperties)
{
    constexpr std::string_view networkAdapters = "NetworkAdapters";
    auto propList = funcBlock.getProperty(networkAdapters.data()).getSelectionValues().asPtrOrNull<IList>();
    int newVal = 0;
    if (propList.getCount() > 0)
        newVal = 1;
    testProperty(networkAdapters.data(), newVal);
}

TEST_F(AsamCmpDataSinkModuleFbFixture, NestedFbCount)
{
    EXPECT_EQ(funcBlock.getFunctionBlocks().getCount(), 2);
}
