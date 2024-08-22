#include <EthLayer.h>
#include <asam_cmp/cmp_header.h>
#include <coreobjects/callable_info_factory.h>

#include <asam_cmp_data_sink/data_sink_fb.h>
#include <asam_cmp_data_sink/data_sink_module_fb.h>
#include <asam_cmp_data_sink/status_fb_impl.h>

#include <asam_cmp_common_lib/ethernet_pcpp_impl.h>
#include <SystemUtils.h>


#include <iostream>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

DataSinkModuleFb::DataSinkModuleFb(const ContextPtr& ctx,
                                           const ComponentPtr& parent,
                                           const StringPtr& localId,
                                           const std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf>& ethernetWrapper)
    : asam_cmp_common_lib::NetworkManagerFb(CreateType(), ctx, parent, localId, ethernetWrapper)
{
    createFbs();
    startCapture();
}

FunctionBlockPtr DataSinkModuleFb::create(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
{
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf> ptr = std::make_shared<asam_cmp_common_lib::EthernetPcppImpl>();
    auto fb = createWithImplementation<IFunctionBlock, DataSinkModuleFb>(ctx, parent, localId, ptr);
    return fb;
}

DataSinkModuleFb::~DataSinkModuleFb()
{
    stopCapture();
}

void DataSinkModuleFb::networkAdapterChangedInternal()
{
    startCapture();
}

FunctionBlockTypePtr DataSinkModuleFb::CreateType()
{
    return FunctionBlockType("asam_cmp_data_sink_module", "DataSinkModule", "ASAM CMP Data Sink Module");
}

void DataSinkModuleFb::createFbs()
{
    const StringPtr statusId = "asam_cmp_status";
    auto newFb = createWithImplementation<IFunctionBlock, StatusFbImpl>(context, functionBlocks, statusId);
    functionBlocks.addItem(newFb);
    auto statusMt = functionBlocks.getItems()[0].asPtr<IStatusHandler>(true)->getStatusMt();

    const StringPtr dataSinkId = "asam_cmp_data_sink";
    newFb = createWithImplementation<IFunctionBlock, DataSinkFb>(context, functionBlocks, dataSinkId, statusMt, callsMap);
    functionBlocks.addItem(newFb);
}

void DataSinkModuleFb::startCapture()
{
    std::scoped_lock lock{sync};

    stopCapture();
    ethernetWrapper->startCapture(
        [this](pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie) { onPacketArrives(packet, dev, cookie); }
    );
}

void DataSinkModuleFb::stopCapture()
{
    ethernetWrapper->stopCapture();
}

void DataSinkModuleFb::onPacketArrives(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)
{
    auto acPackets = decode(packet);

    for (const auto& acPacket : acPackets)
    {
        switch (acPacket->getMessageType())
        {
            case ASAM::CMP::CmpHeader::MessageType::data:
                callsMap.processPacket(acPacket);
                break;
            case ASAM::CMP::CmpHeader::MessageType::status:
                functionBlocks.getItems()[0].asPtr<IStatusHandler>(true)->processStatusPacket(acPacket);
                break;
            default:
                LOG_I("ASAM CMP Message Type {} is not supported", to_underlying(acPacket->getMessageType()));
        }
    }
}

std::vector<std::shared_ptr<ASAM::CMP::Packet>> DataSinkModuleFb::decode(pcpp::RawPacket* packet)
{
    pcpp::Packet parsedPacket(packet);
    pcpp::EthLayer* ethLayer = static_cast<pcpp::EthLayer*>(parsedPacket.getLayerOfType(pcpp::Ethernet));
    assert(ethLayer->getDestMac() == asam_cmp_common_lib::EthernetPcppImpl::broadcastMac);
    assert(pcpp::netToHost16(ethLayer->getEthHeader()->etherType) == asam_cmp_common_lib::EthernetPcppImpl::asamCmpEtherType);

    return decoder.decode(ethLayer->getLayerPayload(), ethLayer->getLayerPayloadSize());
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
