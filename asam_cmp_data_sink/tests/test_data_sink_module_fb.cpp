#include <asam_cmp_data_sink/module_dll.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>
#include <asam_cmp_common_lib/ethernet_pcpp_mock.h>
#include <asam_cmp_common_lib/network_manager_fb.h>
#include <asam_cmp_data_sink/data_sink_module_fb.h>

using namespace daq;
using ::testing::Return;
using namespace testing;

class DataSinkModuleFbTest : public ::testing::Test
{
protected:
    DataSinkModuleFbTest()
    {
        ethernetWrapper = std::make_shared<asam_cmp_common_lib::EthernetPcppMock>();

        names = List<IString>();
        names.pushBack("name1");
        names.pushBack("name2");

        descriptions = List<IString>();
        descriptions.pushBack("desc1");
        descriptions.pushBack("desc2");

        auto startStub = []() { };
        auto stopStub = []() {};

        ON_CALL(*ethernetWrapper, startCapture(_)).WillByDefault(startStub);
        ON_CALL(*ethernetWrapper, stopCapture()).WillByDefault(stopStub);
        ON_CALL(*ethernetWrapper, getEthernetDevicesNamesList()).WillByDefault(Return(names));
        ON_CALL(*ethernetWrapper, getEthernetDevicesDescriptionsList()).WillByDefault(Return(descriptions));

        EXPECT_CALL(*ethernetWrapper, startCapture(_)).Times(AtLeast(1));
        EXPECT_CALL(*ethernetWrapper, stopCapture()).Times(AtLeast(1));
        EXPECT_CALL(*ethernetWrapper, setDevice(_)).Times(AtLeast(1)).WillRepeatedly(Return(true));
        EXPECT_CALL(*ethernetWrapper, getEthernetDevicesNamesList()).Times(AtLeast(1));
        EXPECT_CALL(*ethernetWrapper, getEthernetDevicesDescriptionsList()).Times(AtLeast(1));

        auto logger = Logger();
        context = Context(Scheduler(logger), logger, TypeManager(), nullptr);
                    
        funcBlock = createWithImplementation<IFunctionBlock, modules::asam_cmp_data_sink_module::DataSinkModuleFb>(
            context, nullptr, "id", ethernetWrapper);
    }

protected:
    template <typename T>
    void testProperty(const StringPtr& name, T newValue, bool success = true);

protected:
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppMock> ethernetWrapper;
    ListPtr<StringPtr> names;
    ListPtr<StringPtr> descriptions;

    ContextPtr context;
    FunctionBlockPtr funcBlock;
};

TEST_F(DataSinkModuleFbTest, NotNull)
{
    ASSERT_NE(ethernetWrapper, nullptr);
    ASSERT_NE(context, nullptr);
    ASSERT_NE(funcBlock, nullptr);
}

TEST_F(DataSinkModuleFbTest, FunctionBlockType)
{
    auto type = funcBlock.getFunctionBlockType();
    ASSERT_EQ(type.getId(), "asam_cmp_data_sink_module");
    ASSERT_EQ(type.getName(), "DataSinkModule");
    ASSERT_EQ(type.getDescription(), "ASAM CMP Data Sink Module");
}

template <typename T>
void DataSinkModuleFbTest::testProperty(const StringPtr& name, T newValue, bool success)
{
    funcBlock.setPropertyValue(name, newValue);
    const T value = funcBlock.getPropertyValue(name);
    if (success)
        ASSERT_EQ(value, newValue);
    else
        ASSERT_NE(value, newValue);
}

TEST_F(DataSinkModuleFbTest, NetworkAdaptersProperties)
{
    constexpr std::string_view networkAdapters = "NetworkAdapters";
    auto propList = funcBlock.getProperty(networkAdapters.data()).getSelectionValues().asPtrOrNull<IList>();
    int newVal = 0;
    if (propList.getCount() > 0)
        newVal = 1;
    testProperty(networkAdapters.data(), newVal);
}

TEST_F(DataSinkModuleFbTest, NestedFbCount)
{
    EXPECT_EQ(funcBlock.getFunctionBlocks().getCount(), 2);
}
