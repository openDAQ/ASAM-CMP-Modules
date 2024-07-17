#include <asam_cmp_capture_module/asam_cmp_ethernet_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

ListPtr<StringPtr> AsamCmpEthernetImpl::getEthernatDevicesNamesList()
{
    auto& deviceList = pcapDeviceList.getPcapLiveDevicesList();
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

ListPtr<StringPtr> AsamCmpEthernetImpl::getEthernatDevicesDescriptionsList()
{
    auto& deviceList = pcapDeviceList.getPcapLiveDevicesList();
    ListPtr<StringPtr> devicesDescriptions = List<IString>();
    for (const auto& device : deviceList)
        addDeviceDescription(devicesDescriptions, device->getDesc());

    return devicesDescriptions;
}

pcpp::PcapLiveDevice* AsamCmpEthernetImpl::getPcapLiveDevice(StringPtr deviceName)
{
    return pcapDeviceList.getPcapLiveDeviceByName(deviceName);
}

void AsamCmpEthernetImpl::sendPacket(const std::vector<uint8_t>& data)
{

}

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
