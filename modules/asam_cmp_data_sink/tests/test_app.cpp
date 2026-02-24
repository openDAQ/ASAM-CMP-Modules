#include <EthLayer.h>
#include <Packet.h>
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

    // To prevent memory leak error caused by the PcapPlusPlus library
    pcpp::EthLayer newEthernetLayer(pcpp::MacAddress("00:00:00:00:00:00"), pcpp::MacAddress("00:00:00:00:00:00"), 0);
    pcpp::Packet newPacket;
    newPacket.addLayer(&newEthernetLayer);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    auto res = RUN_ALL_TESTS();

    return res;
}
