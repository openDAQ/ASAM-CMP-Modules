#include <asam_cmp_common_lib/ethernet_pcpp_impl.h>
#include <PcapLiveDeviceList.h>
#include <SystemUtils.h>
#include <EthLayer.h>
#include <PayloadLayer.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

EthernetPcppImpl::EthernetPcppImpl()
    : deviceList(createAvailableDevicesList())
    , activeDevice(getFirstAvailableDevice())
{

}

std::vector<pcpp::PcapLiveDevice*> EthernetPcppImpl::createAvailableDevicesList() const
{
    auto& deviceList = pcapDeviceList.getPcapLiveDevicesList();
    std::vector<pcpp::PcapLiveDevice*> devices;
    for (const auto& device : deviceList)
    {
        try
        {
            auto newDevice = getPcapLiveDevice(device->getName());
            devices.push_back(newDevice);
            newDevice->close();
        }
        catch (...)
        {

        }
    }

    return devices;
}

pcpp::PcapLiveDevice* EthernetPcppImpl::getFirstAvailableDevice() const
{
    for (const auto& device : deviceList)
    {
        try
        {
            return getPcapLiveDevice(device->getName());
        }
        catch (...)
        {
            continue;
        }
    }

    return nullptr;
}

void setFilters(pcpp::PcapLiveDevice* device)
{
    // create a filters
    pcpp::MacAddressFilter macAddressFilter{EthernetPcppImpl::broadcastMac, pcpp::Direction(pcpp::DST)};
    pcpp::EtherTypeFilter ethernetTypeFilter(EthernetPcppImpl::asamCmpEtherType);

    // create an AND filter to combine both filters
    pcpp::AndFilter andFilter;
    andFilter.addFilter(&macAddressFilter);
    andFilter.addFilter(&ethernetTypeFilter);
    // set the filter on the device
    device->setFilter(andFilter);
}

pcpp::PcapLiveDevice* EthernetPcppImpl::getPcapLiveDevice(const StringPtr& deviceName) const
{
    auto pcapLiveDevice = pcapDeviceList.getPcapLiveDeviceByName(deviceName);
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

    return pcapLiveDevice;
}

ListPtr<StringPtr> EthernetPcppImpl::getEthernetDevicesNamesList()
{
    ListPtr<StringPtr> devicesNames = List<IString>();
    for (const auto& device : deviceList)
        devicesNames.pushBack(device->getName());

    return devicesNames;
}

void addDeviceDescription(ListPtr<StringPtr>& devicesNames, const StringPtr& name)
{
    StringPtr newName = name;
    for (size_t index = 1; std::find(devicesNames.begin(), devicesNames.end(), newName) != devicesNames.end(); ++index)
        newName = fmt::format("{} {}", name, index);
    devicesNames.pushBack(newName);
}

ListPtr<StringPtr> EthernetPcppImpl::getEthernetDevicesDescriptionsList()
{
    ListPtr<StringPtr> devicesDescriptions = List<IString>();
    for (const auto& device : deviceList)
        addDeviceDescription(devicesDescriptions, device->getDesc());

    return devicesDescriptions;
}

bool EthernetPcppImpl::setDevice(const StringPtr& deviceName)
{
    pcpp::PcapLiveDevice* oldDevice = activeDevice;
    try
    {
        activeDevice = getPcapLiveDevice(deviceName);
    }
    catch (...)
    {
        activeDevice = oldDevice;
        return false;
    }

    return true;
}

void EthernetPcppImpl::sendPacket(const std::vector<uint8_t>& data)
{
    // create a new Ethernet layer
    pcpp::EthLayer newEthernetLayer(
        pcpp::MacAddress(activeDevice->getMacAddress()), pcpp::MacAddress("FF:FF:FF:FF:FF:FF"), asamCmpEtherType);
    pcpp::PayloadLayer payloadLayer(data.data(), data.size());
    // create a packet with initial capacity of 100 bytes (will grow automatically if needed)
    pcpp::Packet newPacket(100);
    bool res = newPacket.addLayer(&newEthernetLayer);
    assert(res);
    res = newPacket.addLayer(&payloadLayer);
    assert(res);
    // compute all calculated fields
    newPacket.computeCalculateFields();

    activeDevice->sendPacket(&newPacket);
}

void EthernetPcppImpl::startCapture(std::function<void(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)> onPacketReceivedCb)
{
    stopCapture();
    setFilters(activeDevice);

    activeDevice->startCapture(onPacketReceivedCb, nullptr);
}

void EthernetPcppImpl::stopCapture()
{
    if (isDeviceCapturing())
        activeDevice->stopCapture();
}

bool EthernetPcppImpl::isDeviceCapturing() const
{
    return activeDevice->captureActive();
}

END_NAMESPACE_ASAM_CMP_COMMON
