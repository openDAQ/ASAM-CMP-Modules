#include <asam_cmp_capture_module/capture_fb_impl.h>
#include <asam_cmp_capture_module/interface_fb_impl.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <set>
#include <fmt/format.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

CaptureFbImpl::CaptureFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , deviceId(0)
{
    initProperties();
    initEncoders();
}

void CaptureFbImpl::initEncoders()
{
    for (int i = 0; i < encoders.size(); ++i)
    {
        encoders[i].setDeviceId(deviceId);
        encoders[i].setStreamId(i);
    }
}

FunctionBlockTypePtr CaptureFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_capture_fb", "AsamCmpCaptureFb", "Asam CMP Capture Fb");
}

void CaptureFbImpl::initProperties()
{
    StringPtr propName = "DeviceId";
    auto prop = IntPropertyBuilder(propName, deviceId).build();
    objPtr.addProperty(prop);
    objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateDeviceId(); };

    propName = "AddInterface";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>())).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, Procedure([this] { this->addInterfaceInternal(); }));

    propName = "RemoveInterface";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("nInd", ctInt)))).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName,
                                                                       Procedure([this](IntPtr nInd) { removeInterfaceInternal(nInd); }));
}

void CaptureFbImpl::updateDeviceId()
{
    Int newDeviceId = objPtr.getPropertyValue("DeviceId");

    if (newDeviceId == deviceId)
        return;

    if (newDeviceId >= 0 && newDeviceId <= std::numeric_limits<uint16_t>::max())
    {
        deviceId = newDeviceId;
        initEncoders();
    }
    else
    {
        objPtr.setPropertyValue("DeviceId", deviceId);
    }
}

void CaptureFbImpl::addInterfaceInternal(){
    auto newId = interfaceIdManager.getFirstUnusedId();
    InterfaceInit init
    {
        newId, &interfaceIdManager, &streamIdManager, &encoders
    };

    StringPtr fbId = fmt::format("asam_cmp_interface_{}", newId);
    functionBlocks.addItem(createWithImplementation<IFunctionBlock, InterfaceFbImpl>(context, functionBlocks, fbId, init));
    interfaceIdManager.addId(newId);
}

void CaptureFbImpl::removeInterfaceInternal(size_t nInd)
{
    interfaceIdManager.removeId(functionBlocks.getItems().getItemAt(nInd).getPropertyValue("InterfaceId"));
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(nInd));
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
