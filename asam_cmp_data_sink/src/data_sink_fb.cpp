#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>

#include <asam_cmp_data_sink/capture_fb.h>
#include <asam_cmp_data_sink/data_sink_fb.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

DataSinkFb::DataSinkFb(const ContextPtr& ctx,
                       const ComponentPtr& parent,
                       const StringPtr& localId,
                       StatusMt statusMt,
                       DataPacketsPublisher& dataPacketsPublisher,
                       CapturePacketsPublisher& capturePacketsPublisher)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , status(statusMt)
    , dataPacketsPublisher(dataPacketsPublisher)
    , capturePacketsPublisher(capturePacketsPublisher)
{
    initProperties();
}

FunctionBlockTypePtr DataSinkFb::CreateType()
{
    return FunctionBlockType("AsamCmpDataSink", "AsamCmpDataSink", "ASAM CMP Data Sink");
}

void DataSinkFb::addCaptureModuleFromStatus(int index)
{
    std::scoped_lock lock{sync};

    auto deviceStatus = status.getDeviceStatus(index);
    const StringPtr fbId = getFbId(captureModuleId);
    const auto newFb = createWithImplementation<IFunctionBlock, CaptureFb>(
        context, functionBlocks, fbId, dataPacketsPublisher, capturePacketsPublisher, std::move(deviceStatus));
    functionBlocks.addItem(newFb);
    ++captureModuleId;
}

void DataSinkFb::addCaptureModuleEmpty()
{
    std::scoped_lock lock{sync};

    const StringPtr fbId = getFbId(captureModuleId);
    const auto newFb =
        createWithImplementation<IFunctionBlock, CaptureFb>(context, functionBlocks, fbId, dataPacketsPublisher, capturePacketsPublisher);
    functionBlocks.addItem(newFb);
    capturePacketsPublisher.subscribe(newFb.getPropertyValue("DeviceId"), newFb.as<IAsamCmpPacketsSubscriber>(true));
    ++captureModuleId;
}

void DataSinkFb::removeCaptureModule(int fbIndex)
{
    std::scoped_lock lock{sync};

    FunctionBlockPtr captureFb = functionBlocks.getItems().getItemAt(fbIndex);
    uint16_t deviceId = captureFb.getPropertyValue("DeviceId");
    capturePacketsPublisher.unsubscribe(deviceId, captureFb.as<IAsamCmpPacketsSubscriber>(true));

    for (const FunctionBlockPtr& interfaceFb : captureFb.getFunctionBlocks())
    {
        uint32_t interfaceId = interfaceFb.getPropertyValue("InterfaceId");
        for (const auto& streamFb : interfaceFb.getFunctionBlocks())
        {
            uint8_t streamId = static_cast<Int>(streamFb.getPropertyValue("StreamId"));
            auto handler = streamFb.as<IAsamCmpPacketsSubscriber>(true);
            dataPacketsPublisher.unsubscribe({deviceId, interfaceId, streamId}, handler);
        }
    }
    functionBlocks.removeItem(captureFb);
}

StringPtr DataSinkFb::getFbId(size_t id)
{
    return fmt::format("Capture_{}", id);
}

StringPtr DataSinkFb::getFbName(size_t id)
{
    return fmt::format("Capture {}", id);
}

void DataSinkFb::initProperties()
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
