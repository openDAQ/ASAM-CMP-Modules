#include <asam_cmp_data_sink/module_dll.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>

using namespace daq;

class AsamCmpDataSinkFbFixture : public ::testing::Test
{
protected:
    AsamCmpDataSinkFbFixture()
    {
        auto logger = Logger();
        createModule(&module, Context(Scheduler(logger), logger, TypeManager(), nullptr));
        dataSinkModuleFb = module.createFunctionBlock("asam_cmp_data_sink_module", nullptr, "id");
        funcBlock = dataSinkModuleFb.getFunctionBlocks(search::Recursive(search::LocalId("asam_cmp_data_sink")))[0];
    }

protected:
    ModulePtr module;
    FunctionBlockPtr funcBlock;
    FunctionBlockPtr dataSinkModuleFb;
};

TEST_F(AsamCmpDataSinkFbFixture, NotNull)
{
    ASSERT_NE(module, nullptr);
    ASSERT_NE(funcBlock, nullptr);
}

TEST_F(AsamCmpDataSinkFbFixture, FunctionBlockType)
{
    auto type = funcBlock.getFunctionBlockType();
    ASSERT_EQ(type.getId(), "asam_cmp_data_sink");
    ASSERT_EQ(type.getName(), "AsamCmpDataSink");
    ASSERT_EQ(type.getDescription(), "ASAM CMP Data Sink");
}

TEST_F(AsamCmpDataSinkFbFixture, AddCaptureModuleFromStatus)
{
    auto proc = daq::Procedure([]() {});
    EXPECT_THROW(funcBlock.setPropertyValue("AddCaptureModuleFromStatus", proc), daq::AccessDeniedException);

    ProcedurePtr func = funcBlock.getPropertyValue("AddCaptureModuleFromStatus");
    func(0);
    ASSERT_EQ(funcBlock.getFunctionBlocks().getCount(), 1);
}

TEST_F(AsamCmpDataSinkFbFixture, AddCaptureModuleEmpty)
{
    auto proc = daq::Procedure([]() {});
    EXPECT_THROW(funcBlock.setPropertyValue("AddCaptureModuleEmpty", proc), daq::AccessDeniedException);

    ProcedurePtr func = funcBlock.getPropertyValue("AddCaptureModuleEmpty");
    func();
    ASSERT_EQ(funcBlock.getFunctionBlocks().getCount(), 1);
}

TEST_F(AsamCmpDataSinkFbFixture, RemoveCaptureModule)
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
