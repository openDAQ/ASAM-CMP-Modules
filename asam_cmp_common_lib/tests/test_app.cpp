#include <testutils/testutils.h>
#include <testutils/bb_memcheck_listener.h>
#include <coreobjects/util.h>
#include <opendaq/module_manager_init.h>

int main(int argc, char** args)
{
    using namespace daq;

    daqInitializeCoreObjectsTesting();
    daqInitModuleManagerLibrary();

    testing::InitGoogleTest(&argc, args);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    auto res = RUN_ALL_TESTS();

    return  res;
}
