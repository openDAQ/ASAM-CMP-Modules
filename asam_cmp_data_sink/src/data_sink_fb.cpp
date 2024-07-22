#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>

#include <asam_cmp_data_sink/data_sink_fb.h>
#include <asam_cmp_data_sink/capture_fb.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

DataSinkFbImpl::DataSinkFbImpl(const ContextPtr& ctx,
                                             const ComponentPtr& parent,
                                             const StringPtr& localId,
                                             StatusMt statusMt)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , status(statusMt)
{
    initProperties();
}

FunctionBlockTypePtr DataSinkFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_data_sink", "AsamCmpDataSink", "ASAM CMP Data Sink");
}

void DataSinkFbImpl::addCaptureModuleFromStatus(int index)
{
    auto deviceStatus = status.getDeviceStatus(index);
    const StringPtr fbId = fmt::format("capture_module_{}", captureModuleId);
    const auto newFb = createWithImplementation<IFunctionBlock, CaptureModuleFb>(context, functionBlocks, fbId, std::move(deviceStatus));
    functionBlocks.addItem(newFb);
    ++captureModuleId;
}

void DataSinkFbImpl::addCaptureModuleEmpty()
{
    const StringPtr fbId = fmt::format("capture_module_{}", captureModuleId);
    const auto newFb = createWithImplementation<IFunctionBlock, CaptureModuleFb>(context, functionBlocks, fbId);
    functionBlocks.addItem(newFb);
    ++captureModuleId;
}

void DataSinkFbImpl::removeCaptureModule(int fbIndex)
{
    functionBlocks.removeItem(functionBlocks.getItems().getItemAt(fbIndex));
}

void DataSinkFbImpl::initProperties()
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
