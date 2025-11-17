#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>

#include <asam_cmp_data_sink/capture_fb.h>
#include <asam_cmp_data_sink/data_sink_fb.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

DataSinkFb::DataSinkFb(const ModuleInfoPtr& moduleInfo,
                       const ContextPtr& ctx,
                       const ComponentPtr& parent,
                       const StringPtr& localId,
                       StatusMt statusMt,
                       DataPacketsPublisher& dataPacketsPublisher,
                       CapturePacketsPublisher& capturePacketsPublisher)
    : FunctionBlock(CreateType(moduleInfo), ctx, parent, localId)
    , status(statusMt)
    , dataPacketsPublisher(dataPacketsPublisher)
    , capturePacketsPublisher(capturePacketsPublisher)
{
    initProperties();
}

FunctionBlockTypePtr DataSinkFb::CreateType(const ModuleInfoPtr& moduleInfo)
{
    auto fbType =  FunctionBlockType("AsamCmpDataSink", "AsamCmpDataSink", "ASAM CMP Data Sink");
    checkErrorInfo(fbType.asPtr<IComponentTypePrivate>(true)->setModuleInfo(moduleInfo));
    return fbType;
}

void DataSinkFb::addCaptureModuleFromStatus(int index)
{
    auto lock = this->getRecursiveConfigLock();

    auto deviceStatus = status.getDeviceStatus(index);
    const StringPtr fbId = getFbId(captureModuleId);
    const auto newFb = createWithImplementation<IFunctionBlock, CaptureFb>(
        this->type.getModuleInfo(), context, functionBlocks, fbId, dataPacketsPublisher, capturePacketsPublisher, std::move(deviceStatus));
    this->addNestedFunctionBlock(newFb);
    ++captureModuleId;
}

void DataSinkFb::addCaptureModuleEmpty()
{
    auto lock = this->getRecursiveConfigLock();

    const StringPtr fbId = getFbId(captureModuleId);
    const auto newFb = createWithImplementation<IFunctionBlock, CaptureFb>(
            this->type.getModuleInfo(), context, functionBlocks, fbId, dataPacketsPublisher, capturePacketsPublisher);
    this->addNestedFunctionBlock(newFb);
    capturePacketsPublisher.subscribe(newFb.getPropertyValue("DeviceId"), newFb.as<IAsamCmpPacketsSubscriber>(true));
    ++captureModuleId;
}

void DataSinkFb::removeCaptureModule(int fbIndex)
{
    auto lock = this->getRecursiveConfigLock();

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

DictPtr<IString, IFunctionBlockType> DataSinkFb::onGetAvailableFunctionBlockTypes()
{
    auto captureType = CaptureFb::CreateType(this->type.getModuleInfo());
    return Dict<IString, IFunctionBlockType>({{captureType.getId(), captureType}});
}

FunctionBlockPtr DataSinkFb::onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)
{
    if (typeId == CaptureFb::CreateType(this->type.getModuleInfo()).getId())
    {
        addCaptureModuleEmpty();
        auto captureFb = this->functionBlocks.getItems().getItemAt(this->functionBlocks.getItems().getCount() - 1);
        return captureFb;
    }

    throw NotFoundException("Function block not found");
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
