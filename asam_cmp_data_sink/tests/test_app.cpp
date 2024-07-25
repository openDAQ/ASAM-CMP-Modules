#include <testutils/testutils.h>
#include <testutils/bb_memcheck_listener.h>
#include <coreobjects/util.h>
#include <opendaq/module_manager_init.h>
#include <asam_cmp_data_sink/module_dll.h>
#include <opendaq/context_factory.h>
#include <iostream>

int main(int argc, char** args)
{
    using namespace daq;

    daqInitializeCoreObjectsTesting();
    daqInitModuleManagerLibrary();

    testing::InitGoogleTest(&argc, args);

   /* try
    {
        ModulePtr module;
        createModule(&module, NullContext());
        module.createFunctionBlock("asam_cmp_data_sink_module", nullptr, "id");
    }
    catch (std::exception& e)
    {
        std::cerr << e.what();
    }*/

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    auto res = RUN_ALL_TESTS();

    return  res;
}
