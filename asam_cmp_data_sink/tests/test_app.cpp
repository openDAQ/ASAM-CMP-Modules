#include <PcapLiveDeviceList.h>
#include <coreobjects/util.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_manager_init.h>
#include <testutils/bb_memcheck_listener.h>
#include <testutils/testutils.h>

#include <asam_cmp_data_sink/module_dll.h>

int main(int argc, char** args)
{
    using namespace daq;

    daqInitializeCoreObjectsTesting();
    daqInitModuleManagerLibrary();

    testing::InitGoogleTest(&argc, args);

    pcpp::PcapLiveDeviceList::getInstance();
    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    auto res = RUN_ALL_TESTS();

    return res;
}
