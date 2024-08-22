#include <asam_cmp_common_lib/network_manager_fb.h>
#include <asam_cmp_common_lib/ethernet_pcpp_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

NetworkManagerFb::NetworkManagerFb(const FunctionBlockTypePtr& type,
                                   const ContextPtr& ctx,
                                   const ComponentPtr& parent,
                                   const StringPtr& localId,
                                   const std::shared_ptr<EthernetPcppItf>& ethernetWrapper)
    : FunctionBlock(type, ctx, parent, localId)
    , ethernetWrapper(ethernetWrapper)
{
    initProperties();
    selectedEthernetDeviceName = objPtr.getPropertySelectionValue("NetworkAdaptersNames");
}

NetworkManagerFb::~NetworkManagerFb()
{
}

void NetworkManagerFb::initProperties()
{
    addNetworkAdaptersProperty();
}

void NetworkManagerFb::addNetworkAdaptersProperty()
{
    ListPtr<StringPtr> devicesNames = ethernetWrapper->getEthernetDevicesNamesList();
    ListPtr<StringPtr> devicesDescriptions = ethernetWrapper->getEthernetDevicesDescriptionsList();

    int selectedDeviceInd = 0;
    for (int i = 0; i < devicesNames.getCount(); ++i)
    {
        if (ethernetWrapper->setDevice(devicesNames[i]))
        {
            selectedDeviceInd = i;
            selectedEthernetDeviceName = devicesNames[i];
            break;
        }
    }

    StringPtr propName = "NetworkAdaptersNames";
    auto prop = SelectionPropertyBuilder(propName, devicesNames, 0).setVisible(false).build();
    objPtr.addProperty(prop);

    propName = "NetworkAdapters";
    prop = SelectionPropertyBuilder(propName, devicesDescriptions, 0).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) += [this, propName](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
    {
 //       setPropertyValueInternal(
 //           String("InterfaceId").asPtr<IString>(true), BaseObjectPtr(interfaceId).asPtr<IBaseObject>(true), false, false, false);

        StringPtr oldName = objPtr.getPropertySelectionValue("NetworkAdaptersNames");
        int oldInd = objPtr.getPropertyValue("NetworkAdaptersNames");

        setPropertyValueInternal(String("NetworkAdaptersNames"), args.getValue(), false, false, false);
        std::string newName = objPtr.getPropertySelectionValue("NetworkAdaptersNames");

        if (ethernetWrapper->setDevice(newName))
        {
            selectedEthernetDeviceName = newName;
            networkAdapterChangedInternal();
        }
        else
        {
            setPropertyValueInternal(String("NetworkAdaptersNames"), BaseObjectPtr(oldInd), false, false, false);
            setPropertyValueInternal(String("NetworkAdapters"), BaseObjectPtr(oldInd), false, false, false);
        }
    };
}


END_NAMESPACE_ASAM_CMP_COMMON
