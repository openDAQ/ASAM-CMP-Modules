#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/listobject_factory.h>

#include <asam_cmp_common_lib/interface_common_fb.h>
#include <asam_cmp_common_lib/stream_common_fb_impl.h>
#include <fmt/core.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

using ASAM::CMP::PayloadType;

InterfaceCommonFb::InterfaceCommonFb(const ContextPtr& ctx,
                                     const ComponentPtr& parent,
                                     const StringPtr& localId,
                                     const InterfaceCommonInit& init)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , interfaceIdManager(init.interfaceIdManager)
    , interfaceId(init.id)
    , payloadType(0)
    , isUpdating(false)
    , needsPropertyChanged(false)
    , createdStreams(0)
{
    initProperties();
}

FunctionBlockTypePtr InterfaceCommonFb::CreateType()
{
    return FunctionBlockType("AsamCmpInterface", "AsamCmpInterface", "ASAM CMP Interface");
}

void InterfaceCommonFb::initProperties()
{
    StringPtr propName = "InterfaceId";
    auto prop = IntPropertyBuilder(propName, interfaceId)
                    .setMinValue(static_cast<Int>(std::numeric_limits<uint32_t>::min()))
                    .setMaxValue(static_cast<Int>(std::numeric_limits<uint32_t>::max()))
                    .build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChangedIfNotUpdating(); };

    propName = "PayloadType";
    ListPtr<StringPtr> payloadTypes{"Undefined", "CAN", "CAN FD", "Analog"};
    prop = SelectionPropertyBuilder(propName, payloadTypes, 0).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChangedIfNotUpdating(); };

    propName = "AddStream";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>())).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, Procedure([this] { addStream(); }));

    propName = "RemoveStream";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("nInd", ctInt)))).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, Procedure([this](IntPtr nInd) { removeStream(nInd); }));
}

void InterfaceCommonFb::addStream()
{
    std::scoped_lock lock{sync};
    addStreamInternal();
}

void InterfaceCommonFb::removeStream(size_t nInd)
{
    std::scoped_lock lock{sync};
    removeStreamInternal(nInd);
}

void InterfaceCommonFb::updateInterfaceIdInternal()
{
    Int newId = objPtr.getPropertyValue("InterfaceId");

    if (newId == interfaceId)
        return;

    interfaceIdManager->removeId(interfaceId);
    interfaceId = newId;
    interfaceIdManager->addId(interfaceId);
}

void InterfaceCommonFb::updatePayloadTypeInternal()
{
    Int newType = objPtr.getPropertyValue("PayloadType");

    if (newType < 0 || static_cast<size_t>(newType) > maxPayloadIndex())
    {
        setPropertyValueInternal(String("PayloadType").asPtr<IString>(true),
                                 BaseObjectPtr(payloadTypeToIndex(payloadType.getType())).asPtr<IBaseObject>(true),
                                 false,
                                 false,
                                 false);
    }
    else
    {
        payloadType.setType(indexToPayloadType(newType));
        for (const auto& fb : functionBlocks.getItems())
        {
            fb.as<IStreamCommon>(true)->setPayloadType(payloadType);
        }
    }
}

void InterfaceCommonFb::removeStreamInternal(size_t nInd)
{
    auto fb = functionBlocks.getItems().getItemAt(nInd);
    uint8_t streamId = static_cast<Int>(fb.getPropertyValue("StreamId"));
    functionBlocks.removeItem(fb);
    streamIdManager.removeId(streamId);
}

daq::ErrCode INTERFACE_FUNC InterfaceCommonFb::beginUpdate()
{
    daq::ErrCode result = FunctionBlock::beginUpdate();
    if (result == OPENDAQ_SUCCESS)
        isUpdating = true;

    return result;
}

void InterfaceCommonFb::endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating)
{
    std::scoped_lock lock{sync};
    FunctionBlock::endApplyProperties(propsAndValues, parentUpdating);

    if (needsPropertyChanged)
    {
        propertyChanged();
        needsPropertyChanged = false;
    }

    isUpdating = false;
}

void InterfaceCommonFb::propertyChanged()
{
    updateInterfaceIdInternal();
    updatePayloadTypeInternal();
}

void InterfaceCommonFb::propertyChangedIfNotUpdating()
{
    if (!isUpdating)
    {
        std::scoped_lock lock{sync};
        propertyChanged();
    }
    else
        needsPropertyChanged = true;
}

DictPtr<IString, IFunctionBlockType> InterfaceCommonFb::onGetAvailableFunctionBlockTypes()
{
    auto type = StreamCommonFb::CreateType();
    return Dict<IString, IFunctionBlockType>({{type.getId(), type}});
}

FunctionBlockPtr InterfaceCommonFb::onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)
{
    if (typeId == StreamCommonFb::CreateType().getId())
    {
        addStream();
        auto streamFb = this->functionBlocks.getItems().getItemAt(this->functionBlocks.getItems().getCount() - 1);
        return streamFb;
    }

    throw NotFoundException("Function block not found");
}

const static std::unordered_map<uint32_t, Int> payloadTypeToIndexMap = {
    {PayloadType::invalid, 0}, {PayloadType::can, 1}, {PayloadType::canFd, 2}, {PayloadType::analog, 3}};

const std::vector<uint32_t> indexToPayloadTypeMap = []() {
    std::vector<uint32_t> vec;
    vec.resize(payloadTypeToIndexMap.size());
    for (const auto& [key, value] : payloadTypeToIndexMap)
    {
        vec[value] = key;
    }
    return vec;
}();

uint32_t InterfaceCommonFb::indexToPayloadType(Int index) const
{
    if (static_cast<size_t>(index) < indexToPayloadTypeMap.size())
    {
        return indexToPayloadTypeMap[index];
    }
    return PayloadType::invalid;
}

Int InterfaceCommonFb::payloadTypeToIndex(PayloadType type) const
{
    auto it = payloadTypeToIndexMap.find(type.getType());
    if (it != payloadTypeToIndexMap.end())
    {
        return it->second;
    }
    return 0;
}

size_t InterfaceCommonFb::maxPayloadIndex() const
{
    return payloadTypeToIndexMap.size() - 1;
}

END_NAMESPACE_ASAM_CMP_COMMON
