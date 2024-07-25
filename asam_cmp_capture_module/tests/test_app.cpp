#include <testutils/testutils.h>
#include <testutils/bb_memcheck_listener.h>
#include <coreobjects/util.h>
#include <opendaq/module_manager_init.h>
#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/module_dll.h>
#include <opendaq/context_factory.h>
#include <coretypes/stringobject_factory.h>
#include <opendaq/module_ptr.h>


int main(int argc, char** args)
{
    using namespace daq;

    daq::daqInitializeCoreObjectsTesting();
    daqInitModuleManagerLibrary();

    testing::InitGoogleTest(&argc, args);
    
    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    auto res = RUN_ALL_TESTS();

    return  res;
}
