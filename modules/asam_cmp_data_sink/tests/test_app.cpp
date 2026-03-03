#include <opendaq/module_manager_factory.h>

#include <EthLayer.h>
#include <Packet.h>

#include <testutils/testutils.h>
#include <testutils/daq_memcheck_listener.h>

int main(int argc, char** args)
{
    using namespace daq;

    {
        daq::ModuleManager(".");
    }

    testing::InitGoogleTest(&argc, args);

    // To prevent memory leak error caused by the PcapPlusPlus library
    pcpp::EthLayer newEthernetLayer(pcpp::MacAddress("00:00:00:00:00:00"), pcpp::MacAddress("00:00:00:00:00:00"), 0);
    pcpp::Packet newPacket;
    newPacket.addLayer(&newEthernetLayer);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    return RUN_ALL_TESTS();
}
