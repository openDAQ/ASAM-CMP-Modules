#include <asam_cmp/interface_payload.h>
#include <asam_cmp_capture_module/interface_fb.h>
#include <asam_cmp_capture_module/stream_fb.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

#include <iostream>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

InterfaceFb::InterfaceFb(const ContextPtr& ctx,
                         const ComponentPtr& parent,
                         const StringPtr& localId,
                         const asam_cmp_common_lib::InterfaceCommonInit& init,
                         const InterfaceFbInit& internalInit)
    : InterfaceCommonFb(ctx, parent, localId, init)
    , encoders(internalInit.encoders)
    , deviceStatus(internalInit.deviceStatus)
    , statusSync(internalInit.statusSync)
    , vendorDataAsString("")
    , ethernetWrapper(internalInit.ethernetWrapper)
    , allowJumboFrames(internalInit.allowJumboFrames)
    , selectedDeviceName(internalInit.selectedDeviceName)
{
    initProperties();
    initStatusPacket();
    updateInterfaceData();
}

void InterfaceFb::addStreamInternal()
{
    std::scoped_lock lock(statusSync);

    auto newId = streamIdManager.getFirstUnusedId();
    StreamInit internalInit{streamIdsList, statusSync, interfaceId, ethernetWrapper, allowJumboFrames, encoders, [&]() {
                                this->updateInterfaceData();
                            }};
    addStreamWithParams<StreamFb>(newId, internalInit);

    streamIdsList.insert(newId);
    updateInterfaceData();
}

void InterfaceFb::removeStreamInternal(size_t nInd)
{
    std::scoped_lock lock(statusSync);

    int id = functionBlocks.getItems().getItemAt(nInd).getPropertyValue("StreamId");
    streamIdsList.erase(id);
    asam_cmp_common_lib::InterfaceCommonFb::removeStreamInternal(nInd);
    updateInterfaceData();
}

void InterfaceFb::initProperties()
{
    auto propName = "VendorData";
    auto prop = StringPropertyBuilder(propName, vendorDataAsString).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChangedIfNotUpdating(); };
}

void InterfaceFb::updateInterfaceIdInternal()
{
    std::scoped_lock lock(statusSync);
    auto oldId = interfaceId;

    Int newId = objPtr.getPropertyValue("InterfaceId");

    if (interfaceIdManager->isValidId(newId))
    {
        asam_cmp_common_lib::InterfaceCommonFb::updateInterfaceIdInternal();
    }
    else
    {
        setPropertyValueInternal(
            String("InterfaceId").asPtr<IString>(true), BaseObjectPtr(interfaceId).asPtr<IBaseObject>(true), false, false, false);
    }

    if (oldId != interfaceId)
    {
        deviceStatus.removeInterfaceById(oldId);
    }
    updateInterfaceData();
}

void InterfaceFb::updatePayloadTypeInternal()
{
    std::scoped_lock lock(statusSync);

    asam_cmp_common_lib::InterfaceCommonFb::updatePayloadTypeInternal();
    updateInterfaceData();
}

void InterfaceFb::initStatusPacket()
{
    interfaceStatusPacket.setPayload(ASAM::CMP::InterfacePayload());
    interfaceStatusPacket.getPayload().setMessageType(ASAM::CMP::CmpHeader::MessageType::status);
    static_cast<ASAM::CMP::InterfacePayload&>(interfaceStatusPacket.getPayload())
        .setInterfaceStatus(ASAM::CMP::InterfacePayload::InterfaceStatus::linkStatusUp);  // TODO: currently as a constant (may be changed
                                                                                          // in next iterations)

    updateInterfaceData();
}

void InterfaceFb::updateInterfaceData()
{
    static_cast<ASAM::CMP::InterfacePayload&>(interfaceStatusPacket.getPayload()).setInterfaceId(interfaceId);
    static_cast<ASAM::CMP::InterfacePayload&>(interfaceStatusPacket.getPayload()).setInterfaceType(payloadType.getRawPayloadType());

    vendorDataAsString = objPtr.getPropertyValue("VendorData").asPtr<IString>().toStdString();
    vendorData = std::vector<uint8_t>(begin(vendorDataAsString), end(vendorDataAsString));

    std::vector<uint8_t> streamIdsAsVector(begin(streamIdsList), end(streamIdsList));

    static_cast<ASAM::CMP::InterfacePayload&>(interfaceStatusPacket.getPayload())
        .setData(streamIdsAsVector.data(),
                 static_cast<uint16_t>(streamIdsAsVector.size()),
                 vendorData.data(),
                 static_cast<uint16_t>(vendorData.size()));

    deviceStatus.update(interfaceStatusPacket);
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

