#include <asam_cmp_common_lib/capture_common_fb.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/validator_factory.h>
#include <fmt/format.h>
#include <set>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

CaptureCommonFb::CaptureCommonFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , isUpdating(false)
    , needsPropertyChanged(false)
    , createdInterfaces(0)
{
    initProperties();
}

FunctionBlockTypePtr CaptureCommonFb::CreateType()
{
    return FunctionBlockType("capture_module", "CaptureModule", "Capture Module");
}

void CaptureCommonFb::initProperties()
{
    StringPtr propName = "DeviceId";
    auto prop = IntPropertyBuilder(propName, 0).setValidator(Validator("(value >= 0) && (value <= 65535)")).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChangedIfNotUpdating(); };

    propName = "AddInterface";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>())).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, Procedure([this] { addInterfaceInternal(); }));

    propName = "RemoveInterface";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("nInd", ctInt)))).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName,
                                                                       Procedure([this](IntPtr nInd) { removeInterfaceInternal(nInd); }));
}

void CaptureCommonFb::updateDeviceId()
{
    deviceId = objPtr.getPropertyValue("DeviceId");
}

void CaptureCommonFb::removeInterfaceInternal(size_t nInd)
{
    if (isUpdating)
        throw std::runtime_error("Removing pdus is disabled during update");

    interfaceIdManager.removeId(functionBlocks.getItems().getItemAt(nInd).getPropertyValue("InterfaceId"));
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(nInd));
}

daq::ErrCode CaptureCommonFb::beginUpdate()
{
    daq::ErrCode result = FunctionBlock::beginUpdate();
    if (result == OPENDAQ_SUCCESS)
        isUpdating = true;

    return result;
}

void CaptureCommonFb::endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating)
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

void CaptureCommonFb::propertyChanged()
{
    updateDeviceId();
}

void CaptureCommonFb::propertyChangedIfNotUpdating()
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
