#include <EthLayer.h>
#include <asam_cmp/cmp_header.h>
#include <coreobjects/callable_info_factory.h>

#include <asam_cmp_data_sink/data_sink_fb_impl.h>
#include <asam_cmp_data_sink/data_sink_module_fb_impl.h>
#include <asam_cmp_data_sink/status_fb_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

DataSinkModuleFbImpl::DataSinkModuleFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();
    createFbs();
    startCapture();
}

DataSinkModuleFbImpl::~DataSinkModuleFbImpl()
{
    stopCapture();
}

FunctionBlockTypePtr DataSinkModuleFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_data_sink_module", "DataSinkModule", "ASAM CMP Data Sink Module");
}

void DataSinkModuleFbImpl::initProperties()
{
    addNetworkAdaptersProperty();
}

void DataSinkModuleFbImpl::addNetworkAdaptersProperty()
{
    auto& deviceList = pcapDeviceList.getPcapLiveDevicesList();
    ListPtr<StringPtr> devicesDescriptions = List<IString>();
    ListPtr<StringPtr> devicesNames = List<IString>();
    for (const auto& device : deviceList)
    {
        addDeviceDescription(devicesDescriptions, device->getDesc());
        devicesNames.pushBack(device->getName());
    }

    StringPtr propName = "NetworkAdaptersNames";
    auto prop = SelectionPropertyBuilder(propName, devicesNames, 0).setVisible(false).build();
    objPtr.addProperty(prop);

    propName = "NetworkAdapters";
    prop = SelectionPropertyBuilder(propName, devicesDescriptions, 0).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) += [this, propName](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
    {
        objPtr.setPropertyValue("NetworkAdaptersNames", objPtr.getPropertyValue(propName));
        startCapture();
    };
}

void DataSinkModuleFbImpl::createFbs()
{
    const StringPtr statusId = "asam_cmp_status";
    auto newFb = createWithImplementation<IFunctionBlock, StatusFbImpl>(context, functionBlocks, statusId);
    functionBlocks.addItem(newFb);
    auto statusMt = functionBlocks.getItems()[0].asPtr<IStatusHandler>(true)->getStatusMt();

    const StringPtr dataSinkId = "asam_cmp_data_sink";
    newFb = createWithImplementation<IFunctionBlock, DataSinkFbImpl>(context, functionBlocks, dataSinkId, statusMt);
    functionBlocks.addItem(newFb);
}

void DataSinkModuleFbImpl::startCapture()
{
    std::scoped_lock lock{sync};

    stopCapture();

    std::string deviceName = objPtr.getPropertySelectionValue("NetworkAdaptersNames");
    pcapLiveDevice = pcapDeviceList.getPcapLiveDeviceByName(deviceName);
    if (!pcapLiveDevice)
    {
        std::string err = fmt::format("Can't find device {}", deviceName);
        throw std::invalid_argument(err);
    }
    if (!pcapLiveDevice->open())
    {
        std::string err = fmt::format("Can't open device {}", deviceName);
        throw std::invalid_argument(err);
    }

    // create a filters
    pcpp::MacAddressFilter macAddressFilter{broadcastMac, pcpp::Direction(pcpp::DST)};
    pcpp::EtherTypeFilter ethernetTypeFilter(asamCmpEtherType);

    // create an AND filter to combine both filters
    pcpp::AndFilter andFilter;
    andFilter.addFilter(&macAddressFilter);
    andFilter.addFilter(&ethernetTypeFilter);
    // set the filter on the device
    pcapLiveDevice->setFilter(andFilter);

    pcapLiveDevice->startCapture(
        [this](pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie) { onPacketArrives(packet, dev, cookie); }, nullptr);
}

void DataSinkModuleFbImpl::stopCapture()
{
    if (pcapLiveDevice)
        pcapLiveDevice->stopCapture();
}

void DataSinkModuleFbImpl::onPacketArrives(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)
{
    auto acPackets = decode(packet);

    for (const auto& acPacket : acPackets)
    {
        switch (acPacket->getMessageType())
        {
            case ASAM::CMP::CmpHeader::MessageType::data:
                break;
            case ASAM::CMP::CmpHeader::MessageType::status:
                functionBlocks.getItems()[0].asPtr<IStatusHandler>(true)->processStatusPacket(acPacket);
                break;
            default:
                LOG_I("ASAM CMP Message Type {} is not supported", to_underlying(acPacket->getMessageType()));
        }
    }
}

std::vector<std::shared_ptr<ASAM::CMP::Packet>> DataSinkModuleFbImpl::decode(pcpp::RawPacket* packet)
{
    pcpp::Packet parsedPacket(packet);
    pcpp::EthLayer* ethLayer = static_cast<pcpp::EthLayer*>(parsedPacket.getLayerOfType(pcpp::Ethernet));
    assert(ethLayer->getDestMac() == broadcastMac);
    assert(ethLayer->getEthHeader()->etherType == asamCmpEtherType);

    return decoder.decode(ethLayer->getLayerPayload(), ethLayer->getLayerPayloadSize());
}

void DataSinkModuleFbImpl::addDeviceDescription(ListPtr<StringPtr>& devicesNames, const StringPtr& name)
{
    StringPtr newName = name;
    for (size_t index = 1; std::find(devicesNames.begin(), devicesNames.end(), newName) != devicesNames.end(); ++index)
        newName = fmt::format("{} {}", name, index);
    devicesNames.pushBack(newName);
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
