#include <EthLayer.h>
#include <asam_cmp/cmp_header.h>
#include <coreobjects/callable_info_factory.h>
#include <opendaq/component_type_private.h>

#include <asam_cmp_data_sink/data_sink_fb.h>
#include <asam_cmp_data_sink/data_sink_module_fb.h>
#include <asam_cmp_data_sink/status_fb_impl.h>

#include <SystemUtils.h>
#include <asam_cmp_common_lib/ethernet_pcpp_impl.h>
#include <Packet.h>

#include <iostream>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

DataSinkModuleFb::DataSinkModuleFb(const ModuleInfoPtr& moduleInfo,
                                   const ContextPtr& ctx,
                                   const ComponentPtr& parent,
                                   const StringPtr& localId,
                                   const std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf>& ethernetWrapper)
    : asam_cmp_common_lib::NetworkManagerFb(CreateType(moduleInfo), ctx, parent, localId, ethernetWrapper)
{
    createFbs();
    startCapture();
}

FunctionBlockPtr DataSinkModuleFb::create(const ModuleInfoPtr& moduleInfo,
                                          const ContextPtr& ctx,
                                          const ComponentPtr& parent,
                                          const StringPtr& localId)
{
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf> ptr = std::make_shared<asam_cmp_common_lib::EthernetPcppImpl>();
    auto fb = createWithImplementation<IFunctionBlock, DataSinkModuleFb>(moduleInfo, ctx, parent, localId, ptr);
    return fb;
}

DataSinkModuleFb::~DataSinkModuleFb()
{
    stopCapture();
}

ErrCode INTERFACE_FUNC DataSinkModuleFb::remove()
{
    stopCapture();
    return Super::remove();
}

void DataSinkModuleFb::networkAdapterChangedInternal()
{
    stopCapture();
    NetworkManagerFb::networkAdapterChangedInternal();
    startCapture();
}

FunctionBlockTypePtr DataSinkModuleFb::CreateType(const ModuleInfoPtr& moduleInfo)
{
    auto fbType = FunctionBlockType("AsamCmpDataSinkModule", "AsamCmpDataSinkModule", "ASAM CMP Data Sink Module");
    checkErrorInfo(fbType.asPtr<IComponentTypePrivate>(true)->setModuleInfo(moduleInfo));
    return fbType;
}

void DataSinkModuleFb::createFbs()
{
    const StringPtr statusId = "Status";
    auto newFb = createWithImplementation<IFunctionBlock, StatusFbImpl>(
        this->type.getModuleInfo(), context, functionBlocks, statusId);
    functionBlocks.addItem(newFb);
    auto statusMt = functionBlocks.getItems()[0].asPtr<IStatusHandler>(true)->getStatusMt();

    const StringPtr dataSinkId = "DataSink";
    newFb = createWithImplementation<IFunctionBlock, DataSinkFb>(
        this->type.getModuleInfo(), context, functionBlocks, dataSinkId, statusMt, dataPacketsPublisher, capturePacketsPublisher);
    functionBlocks.addItem(newFb);
}

void DataSinkModuleFb::startCapture()
{
    auto lock = this->getRecursiveConfigLock();

    ethernetWrapper->startCapture([this](pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)
                                  { onPacketArrives(packet, dev, cookie); });
    captureStartedOnThisFb = true;
}

void DataSinkModuleFb::stopCapture()
{
    if (captureStartedOnThisFb)
    {
        ethernetWrapper->stopCapture();
        captureStartedOnThisFb = false;
    }
}

void DataSinkModuleFb::onPacketArrives(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)
{
    auto acPackets = decode(packet);
    if (acPackets.empty())
        return;

    // "Aggregation of multiple CMP Messages can be realized for different DATA_MESSAGE_PAYLOAD_TYPEs"
    // We can process multiple packets simultaneously only if they are of the same type and IDs

    auto payloadType = acPackets.front()->getPayload().getType();
    auto deviceId = acPackets.front()->getDeviceId();
    auto interfaceId = acPackets.front()->getInterfaceId();
    auto streamId = acPackets.front()->getStreamId();
    auto samePayloadType = std::all_of(acPackets.begin(),
                                       acPackets.end(),
                                       [&](const auto& packet)
                                       {
                                           return packet->getPayload().getType() == payloadType && packet->getDeviceId() == deviceId &&
                                                  packet->getInterfaceId() == interfaceId && packet->getStreamId() == streamId;
                                       });

    if (acPackets.front()->getMessageType() == ASAM::CMP::CmpHeader::MessageType::data && samePayloadType)
    {
        dataPacketsPublisher.publish({deviceId, interfaceId, streamId}, acPackets);
    }
    else
    {
        for (const auto& acPacket : acPackets)
        {
            switch (acPacket->getMessageType())
            {
                case ASAM::CMP::CmpHeader::MessageType::data:
                    dataPacketsPublisher.publish({deviceId, interfaceId, streamId}, acPacket);
                    break;
                case ASAM::CMP::CmpHeader::MessageType::status:
                    functionBlocks.getItems()[0].asPtr<IStatusHandler>(true)->processStatusPacket(acPacket);
                    if (payloadType == ASAM::CMP::PayloadType::cmStatMsg)
                        capturePacketsPublisher.publish(deviceId, acPacket);
                    break;
                default:
                    LOG_I("ASAM CMP Message Type {} is not supported", to_underlying(acPacket->getMessageType()));
            }
        }
    }
}

std::vector<std::shared_ptr<ASAM::CMP::Packet>> DataSinkModuleFb::decode(pcpp::RawPacket* packet)
{
    pcpp::Packet parsedPacket(packet);
    pcpp::EthLayer* ethLayer = static_cast<pcpp::EthLayer*>(parsedPacket.getLayerOfType(pcpp::Ethernet));
    assert(pcpp::netToHost16(ethLayer->getEthHeader()->etherType) == asam_cmp_common_lib::EthernetPcppImpl::asamCmpEtherType);

    return decoder.decode(ethLayer->getLayerPayload(), ethLayer->getLayerPayloadSize());
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
