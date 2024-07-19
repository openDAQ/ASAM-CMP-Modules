#include <asam_cmp_data_sink/capture_module_common_impl.h>
#include <asam_cmp_data_sink/interface_fb_impl.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <fmt/format.h>
#include <set>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

CaptureModuleCommonImpl::CaptureModuleCommonImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();
}

FunctionBlockTypePtr CaptureModuleCommonImpl::CreateType()
{
    return FunctionBlockType("capture_module", "CaptureModule", "Capture Module");
}

void CaptureModuleCommonImpl::initProperties()
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

void CaptureModuleCommonImpl::updateDeviceId()
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

void CaptureModuleCommonImpl::removeInterfaceInternal(size_t nInd)
{
    interfaceIdManager.removeId(functionBlocks.getItems().getItemAt(nInd).getPropertyValue("InterfaceId"));
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(nInd));
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
