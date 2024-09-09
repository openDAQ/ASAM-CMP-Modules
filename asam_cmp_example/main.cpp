#include <opendaq/instance_factory.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/data_packet.h>
#include <opendaq/data_packet_ptr.h>

#include <iostream>

using namespace daq;

int main()
{
    const auto instance = Instance();
    const auto device = instance.addDevice("daqref://device0");

    auto asamCmpCaptureModule = instance.addFunctionBlock("asam_cmp_capture_module_fb");
    auto asamCmpDataSink = instance.addFunctionBlock("asam_cmp_data_sink_module");

    if (asamCmpCaptureModule.getPropertySelectionValue("NetworkAdapters") != asamCmpDataSink.getPropertySelectionValue("NetworkAdapters"))
    {
        asamCmpDataSink.setPropertyValue("NetworkAdapters", asamCmpCaptureModule.getPropertyValue("NetworkAdapters"));
    }

    auto captureFb = asamCmpCaptureModule.getFunctionBlocks(
        search::Custom([](const ComponentPtr& comp) { return comp.getLocalId() == "asam_cmp_capture_fb"; }))[0];

    ProcedurePtr createItfProc = captureFb.getPropertyValue("AddInterface");
    createItfProc();
    auto itf = captureFb.getFunctionBlocks()[0];
    itf.setPropertyValue("PayloadType", 3);

    ProcedurePtr createStreamProc = itf.getPropertyValue("AddStream");
    createStreamProc();
    createStreamProc();

    for (int i = 0; i < 2; ++i)
    {
        auto streamFb = itf.getFunctionBlocks()[i];
        InputPortPtr streamInputPort = streamFb.getInputPorts().getItemAt(0);
        streamInputPort.connect(device.getChannels()[i].getSignals()[0]);
    }


    auto dataSinkFb =
        asamCmpDataSink.getFunctionBlocks(search::Custom([](const ComponentPtr& comp) { return comp.getLocalId() == "asam_cmp_data_sink"; }))[0];
    ProcedurePtr buildFromStatusProc = dataSinkFb.getPropertyValue("AddCaptureModuleFromStatus");

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    buildFromStatusProc(0);

    auto sinkCaptureFb = dataSinkFb.getFunctionBlocks()[0];
    auto sinkItf = sinkCaptureFb.getFunctionBlocks()[0];

    FunctionBlockPtr renderer = instance.addFunctionBlock("RefFBModuleRenderer");
    renderer.setPropertyValue("ShowLastValue", true);
    renderer.setPropertyValue("UseCustomMinMaxValue", true);
    for (int i = 0; i < 2; ++i)
    {
        renderer.getInputPorts()[i].connect(sinkItf.getFunctionBlocks()[i].getSignals()[0]);
    }

    for (int i = 0; i < 2; ++i)
    {
        renderer.getInputPorts()[i + 2].connect(device.getChannels()[i].getSignals()[0]);
    }

	std::this_thread::sleep_for(std::chrono::milliseconds(10000));

	return 0;
}
