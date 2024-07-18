#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>

#include <asam_cmp_data_sink/asam_cmp_data_sink_fb_impl.h>
#include <asam_cmp_data_sink/capture_module_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

AsamCmpDataSinkFbImpl::AsamCmpDataSinkFbImpl(const ContextPtr& ctx,
                                             const ComponentPtr& parent,
                                             const StringPtr& localId,
                                             StatusMt statusMt)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , status(statusMt)
{
    initProperties();
}

FunctionBlockTypePtr AsamCmpDataSinkFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_data_sink", "AsamCmpDataSink", "ASAM CMP Data Sink");
}

void AsamCmpDataSinkFbImpl::addCaptureModuleFromStatus(int index)
{
    auto deviceStatus = status.getDeviceStatus(index);
    const StringPtr fbId = "capture_module";
    auto newFb = createWithImplementation<IFunctionBlock, CaptureModuleImpl>(context, functionBlocks, fbId, std::move(deviceStatus));
    functionBlocks.addItem(newFb);
}

void AsamCmpDataSinkFbImpl::addCaptureModuleEmpty()
{
    const StringPtr fbId = "capture_module";
    auto newFb = createWithImplementation<IFunctionBlock, CaptureModuleImpl>(context, functionBlocks, fbId);
    functionBlocks.addItem(newFb);
}

void AsamCmpDataSinkFbImpl::removeCaptureModule(int fbIndex)
{
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(fbIndex));
}

void AsamCmpDataSinkFbImpl::initProperties()
{
    StringPtr propName = "AddCaptureModuleFromStatus";
    auto arguments = List<IArgumentInfo>(ArgumentInfo("index", ctInt));
    objPtr.addProperty(FunctionPropertyBuilder(propName, ProcedureInfo(arguments)).setReadOnly(true).build());
    auto proc = Procedure([this](IntPtr nItem) { addCaptureModuleFromStatus(nItem); });
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, proc);

    propName = "AddCaptureModuleEmpty";
    objPtr.addProperty(FunctionPropertyBuilder(propName, ProcedureInfo()).setReadOnly(true).build());
    proc = Procedure([this]() { addCaptureModuleEmpty(); });
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, proc);

    propName = "RemoveCaptureModule";
    arguments = List<IArgumentInfo>(ArgumentInfo("fbIndex", ctInt));
    objPtr.addProperty(FunctionPropertyBuilder(propName, ProcedureInfo(arguments)).setReadOnly(true).build());
    proc = Procedure([this](IntPtr nItem) { removeCaptureModule(nItem); });
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(propName, proc);
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
