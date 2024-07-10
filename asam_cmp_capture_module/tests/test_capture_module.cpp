#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/module_dll.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <gtest/gtest.h>

using CaptureModuleTest = testing::Test;
using namespace daq;

static FunctionBlockPtr createAsamCmpCapture()
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, Context(Scheduler(logger), logger, nullptr, nullptr, nullptr));

    auto fb = module.createFunctionBlock("asam_cmp_capture", nullptr, "id");

    return fb;
}

TEST_F(CaptureModuleTest, CreateCaptureModule)
{
    auto asamCmpCapture = createAsamCmpCapture();
    ASSERT_NE(asamCmpCapture, nullptr);
}
