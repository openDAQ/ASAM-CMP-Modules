#include <asam_cmp_common_lib/capture_common_fb.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <fmt/format.h>
#include <set>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

CaptureCommonFb::CaptureCommonFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
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
    auto prop = IntPropertyBuilder(propName, 0).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateDeviceId(); };

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
    Int newDeviceId = objPtr.getPropertyValue("DeviceId");

    if (newDeviceId == deviceId)
        return;

    if (newDeviceId >= 0 && newDeviceId <= std::numeric_limits<uint16_t>::max())
    {
        deviceId = newDeviceId;
    }
    else
    {
        objPtr.setPropertyValue("DeviceId", deviceId);
    }
}

void CaptureCommonFb::removeInterfaceInternal(size_t nInd)
{
    interfaceIdManager.removeId(functionBlocks.getItems().getItemAt(nInd).getPropertyValue("InterfaceId"));
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(nInd));
}

END_NAMESPACE_ASAM_CMP_COMMON
