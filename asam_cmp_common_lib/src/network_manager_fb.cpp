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

    StringPtr propName = "NetworkAdaptersNames";
    auto prop = SelectionPropertyBuilder(propName, devicesNames, 0).setVisible(false).build();
    objPtr.addProperty(prop);

    propName = "NetworkAdapters";
    prop = SelectionPropertyBuilder(propName, devicesDescriptions, 0).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) += [this, propName](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
    {
        objPtr.setPropertyValue("NetworkAdaptersNames", objPtr.getPropertyValue(propName));
        selectedEthernetDeviceName = objPtr.getPropertySelectionValue("NetworkAdaptersNames");
        networkAdapterChangedInternal();
    };
}


END_NAMESPACE_ASAM_CMP_COMMON
