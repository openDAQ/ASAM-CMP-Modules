#include <asam_cmp_capture_module/asam_cmp_capture_module_fb_impl.h>
#include <string_view>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

constexpr std::string_view InputDisconnected{"Disconnected"};
constexpr std::string_view InputConnected{"Connected"};
constexpr std::string_view InputInvalid{"Invalid"};

AsamCmpCaptureModuleFbImpl::AsamCmpCaptureModuleFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();
    //initStatuses();
}

FunctionBlockTypePtr AsamCmpCaptureModuleFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_capture_module", "AsamCmpCaptureModule", "Asam CMP CAptureModule");
}

void AsamCmpCaptureModuleFbImpl::initProperties()
{
    // propName = "SetDevice";
    // prop = FunctionPropertyBuilder(propName, FunctionInfo(ctString, List<IArgumentInfo>(ctStruct))).setReadOnly(true).build();
    // objPtr.addProperty(prop);
    // objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, Function([this](DevicePtr /*device*/) { return this->updateConfigString(); }));


}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
