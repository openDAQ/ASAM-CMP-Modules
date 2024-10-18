/*
 * Copyright 2022-2023 Blueberry d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <opendaq/function_block_impl.h>

#include <asam_cmp_common_lib/common.h>
#include <asam_cmp_common_lib/id_manager.h>
#include <asam_cmp_common_lib/interface_common_fb.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

template <typename... Interfaces>
class CaptureCommonFbImpl : public FunctionBlockImpl<IFunctionBlock, Interfaces...>
{
public:
    explicit CaptureCommonFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~CaptureCommonFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

protected:
    void initDeviceInfoProperties(bool readOnly);
    template <class Impl, typename... Params>
    FunctionBlockPtr addInterfaceWithParams(uint32_t interfaceId, Params&&... params);
    virtual void addInterfaceInternal() = 0;
    virtual void updateDeviceIdInternal();
    virtual void updateDeviceInfoInternal();
    virtual void removeInterfaceInternal(size_t nInd);

    daq::ErrCode INTERFACE_FUNC beginUpdate() override;
    daq::ErrCode INTERFACE_FUNC endUpdate() override;
    virtual void propertyChanged();
    void propertyChangedIfNotUpdating();

private:
    void initProperties();
    void addInterface();
    void removeInterface(size_t nInd);

protected:
    InterfaceIdManager interfaceIdManager;

    uint16_t deviceId{0};
    StringPtr deviceDescription{""};
    StringPtr serialNumber{""};
    StringPtr hardwareVersion{""};
    StringPtr softwareVersion{""};
    std::string vendorDataAsString{""};
    std::vector<uint8_t> vendorData;

    std::atomic_bool isUpdating{false};
    std::atomic_bool needsPropertyChanged{false};

private:
    size_t createdInterfaces{0};
};

using CaptureCommonFb = CaptureCommonFbImpl<>;

template <typename... Interfaces>
CaptureCommonFbImpl<Interfaces...>::CaptureCommonFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlockImpl<IFunctionBlock, Interfaces...>(CreateType(), ctx, parent, localId)
{
    initProperties();
}

template <typename... Interfaces>
FunctionBlockTypePtr CaptureCommonFbImpl<Interfaces...>::CreateType()
{
    return FunctionBlockType("asam_cmp_capture", "AsamCmpCapture", "ASAM CMP Capture");
}

template <typename... Interfaces>
void CaptureCommonFbImpl<Interfaces...>::initProperties()
{
    StringPtr propName = "DeviceId";
    auto prop = IntPropertyBuilder(propName, 0)
                    .setMinValue(static_cast<Int>(std::numeric_limits<uint16_t>::min()))
                    .setMaxValue(static_cast<Int>(std::numeric_limits<uint16_t>::max()))
                    .build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChangedIfNotUpdating(); };

    propName = "AddInterface";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>())).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, Procedure([this] { addInterface(); }));

    propName = "RemoveInterface";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("nInd", ctInt)))).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, Procedure([this](IntPtr nInd) { removeInterface(nInd); }));
}

template <typename... Interfaces>
void CaptureCommonFbImpl<Interfaces...>::initDeviceInfoProperties(bool readOnly)
{
    StringPtr propName = "DeviceDescription";
    auto prop = StringPropertyBuilder(propName, "").setReadOnly(readOnly).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChangedIfNotUpdating(); };

    propName = "SerialNumber";
    prop = StringPropertyBuilder(propName, "").setReadOnly(readOnly).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChangedIfNotUpdating(); };

    propName = "HardwareVersion";
    prop = StringPropertyBuilder(propName, "").setReadOnly(readOnly).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChangedIfNotUpdating(); };

    propName = "SoftwareVersion";
    prop = StringPropertyBuilder(propName, "").setReadOnly(readOnly).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChangedIfNotUpdating(); };

    propName = "VendorData";
    prop = StringPropertyBuilder(propName, "").setReadOnly(readOnly).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChangedIfNotUpdating(); };
}

template <typename... Interfaces>
template <class Impl, typename... Params>
FunctionBlockPtr CaptureCommonFbImpl<Interfaces...>::addInterfaceWithParams(uint32_t interfaceId, Params&&... params)
{
    if (isUpdating)
        throw std::runtime_error("Adding interfaces is disabled during update");
    InterfaceCommonInit init{interfaceId, &interfaceIdManager};

    StringPtr fbId = fmt::format("asam_cmp_interface_{}", createdInterfaces++);
    auto newFb = createWithImplementation<IFunctionBlock, Impl>(context, functionBlocks, fbId, init, std::forward<Params>(params)...);
    functionBlocks.addItem(newFb);
    interfaceIdManager.addId(interfaceId);

    return newFb;
}

template <typename... Interfaces>
void CaptureCommonFbImpl<Interfaces...>::addInterface()
{
    std::scoped_lock lock{sync};
    addInterfaceInternal();
}

template <typename... Interfaces>
void CaptureCommonFbImpl<Interfaces...>::removeInterface(size_t nInd)
{
    std::scoped_lock lock{sync};
    removeInterfaceInternal(nInd);
}

template <typename... Interfaces>
void CaptureCommonFbImpl<Interfaces...>::updateDeviceIdInternal()
{
    deviceId = objPtr.getPropertyValue("DeviceId");
}

template <typename... Interfaces>
void CaptureCommonFbImpl<Interfaces...>::updateDeviceInfoInternal()
{
    deviceDescription = objPtr.getPropertyValue("DeviceDescription");
    serialNumber = objPtr.getPropertyValue("SerialNumber");
    hardwareVersion = objPtr.getPropertyValue("HardwareVersion");
    softwareVersion = objPtr.getPropertyValue("SoftwareVersion");
    vendorDataAsString = objPtr.getPropertyValue("VendorData").asPtr<IString>().toStdString();
    vendorData = std::vector<uint8_t>(begin(vendorDataAsString), end(vendorDataAsString));
}

template <typename... Interfaces>
void CaptureCommonFbImpl<Interfaces...>::removeInterfaceInternal(size_t nInd)
{
    if (isUpdating)
        throw std::runtime_error("Removing interfaces is disabled during update");

    interfaceIdManager.removeId(functionBlocks.getItems().getItemAt(nInd).getPropertyValue("InterfaceId"));
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(nInd));
}

template <typename... Interfaces>
daq::ErrCode CaptureCommonFbImpl<Interfaces...>::beginUpdate()
{
    daq::ErrCode result = FunctionBlockImpl::beginUpdate();
    if (result == OPENDAQ_SUCCESS)
        isUpdating = true;

    return result;
}

template <typename... Interfaces>
inline daq::ErrCode INTERFACE_FUNC CaptureCommonFbImpl<Interfaces...>::endUpdate()
{
    std::scoped_lock lock{sync};
    auto result = FunctionBlockImpl::endUpdate();

    if (needsPropertyChanged)
    {
        propertyChanged();
        needsPropertyChanged = false;
    }

    isUpdating = false;

    return result;
}

template <typename... Interfaces>
void CaptureCommonFbImpl<Interfaces...>::propertyChanged()
{
    updateDeviceIdInternal();
    updateDeviceInfoInternal();
}

template <typename... Interfaces>
void CaptureCommonFbImpl<Interfaces...>::propertyChangedIfNotUpdating()
{
    if (!isUpdating)
    {
        std::scoped_lock lock{sync};
        propertyChanged();
    }
    else
        needsPropertyChanged = true;
}

END_NAMESPACE_ASAM_CMP_COMMON
