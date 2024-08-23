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

class BufferFb : public FunctionBlock
{
public:
    explicit BufferFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : FunctionBlock(CreateType(), ctx, parent, localId)
    {
        createInputPorts();
    }

    void onPacketReceived(const InputPortPtr& port) override
    {
        std::scoped_lock lock(sync);

        const auto conn = port.getConnection();
        if (!conn.assigned())
            return;

        auto packet = conn.dequeue();
        while (packet.assigned())
        {
            const auto packetType = packet.getType();
            if (packetType == PacketType::Event)
            {

            }
            else if (packetType == PacketType::Data)
            {
                DataPacketPtr dataPacket = packet.asPtr<IDataPacket>();
                const auto domainPacket = dataPacket.getDomainPacket();
                if (domainPacket.assigned())
                {
                    if (dataPacket.getDataDescriptor().getSampleType() == SampleType::Struct)
                    {
                        std::cout << "Received packet: " + dataPacket.getDataDescriptor().getName().toStdString() +
                            ", samplesCount: " + std::to_string(dataPacket.getSampleCount()) << std::endl;
                    }
                }
            }

            packet = conn.dequeue();
        }
    }

    
    FunctionBlockTypePtr CreateType()
    {
        return FunctionBlockType("BufferFb", "Buffer", "Register received packets");
    }

    void createInputPorts()
    {
        inputPort = createAndAddInputPort("voltage", PacketReadyNotification::Scheduler);
    }

private:
    InputPortPtr inputPort;
};

int main()
{
    const auto instance = Instance();

    const auto device = instance.addDevice("daqref://device0");
    device.setPropertyValue("EnableCANChannel", True);
    const ChannelPtr canChannel = device.getChannels(search::Custom([](const ComponentPtr& comp) { return comp.getLocalId() == "refcanch"; }))[0];
    const auto canSignal = canChannel.getSignals()[0];

    auto asamCmpDataSink = instance.addFunctionBlock("asam_cmp_data_sink_module");


    instance.removeFunctionBlock(asamCmpDataSink);

    auto asamCmpCaptureModule = instance.addFunctionBlock("asam_cmp_capture_module_fb");
    asamCmpDataSink = instance.addFunctionBlock("asam_cmp_data_sink_module");

    if (asamCmpCaptureModule.getPropertySelectionValue("NetworkAdapters") != asamCmpDataSink.getPropertySelectionValue("NetworkAdapters"))
    {
        asamCmpDataSink.setPropertyValue("NetworkAdapters", asamCmpCaptureModule.getPropertyValue("NetworkAdapters"));
    }

    auto captureFb = asamCmpCaptureModule.getFunctionBlocks(
        search::Custom([](const ComponentPtr& comp) { return comp.getLocalId() == "asam_cmp_capture_fb"; }))[0];

    ProcedurePtr createItfProc = captureFb.getPropertyValue("AddInterface");
    createItfProc();
    auto itf = captureFb.getFunctionBlocks()[0];
    itf.setPropertyValue("PayloadType", 1);

    ProcedurePtr createStreamProc = itf.getPropertyValue("AddStream");
    createStreamProc();
    auto streamFb = itf.getFunctionBlocks()[0];

    InputPortPtr streamInputPort = streamFb.getInputPorts().getItemAt(0);
    streamInputPort.connect(canSignal);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    auto dataSinkFb =
        asamCmpDataSink.getFunctionBlocks(search::Custom([](const ComponentPtr& comp) { return comp.getLocalId() == "asam_cmp_data_sink"; }))[0];
    ProcedurePtr buildFromStatusProc = dataSinkFb.getPropertyValue("AddCaptureModuleFromStatus");

    buildFromStatusProc(0);

    auto sinkCaptureFb = dataSinkFb.getFunctionBlocks()[0];
    auto sinkItf = sinkCaptureFb.getFunctionBlocks()[0];
    auto sinkStreamFb = sinkItf.getFunctionBlocks()[0];

    auto sinkSignal = sinkStreamFb.getSignals()[0];

    auto buffer = createWithImplementation<IFunctionBlock, BufferFb>(instance.getContext(), nullptr, "buffer_fb");

    auto bufferInputPort = buffer.getInputPorts()[0];

    bufferInputPort.connect(sinkSignal);

	std::this_thread::sleep_for(std::chrono::milliseconds(10000));

	return 0;
}
