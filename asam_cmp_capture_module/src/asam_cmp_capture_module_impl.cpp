#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <asam_cmp_capture_module/asam_cmp_capture_module_impl.h>
#include <asam_cmp_capture_module/version.h>
#include <asam_cmp_capture_module/asam_cmp_capture_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

AsamCmpCaptureModule::AsamCmpCaptureModule(ContextPtr ctx)
    : Module("ASAM CMP CaptureModule",
             daq::VersionInfo(ASAM_CMP_CAPTURE_MODULE_MAJOR_VERSION, ASAM_CMP_CAPTURE_MODULE_MINOR_VERSION, ASAM_CMP_CAPTURE_MODULE_PATCH_VERSION),
             std::move(ctx),
             "AsamCmpCaptureModule")
{
}

DictPtr<IString, IFunctionBlockType> AsamCmpCaptureModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    auto typeStatistics = AsamCmpCaptureImpl::CreateType();
    types.set(typeStatistics.getId(), typeStatistics);

    return types;
}

FunctionBlockPtr AsamCmpCaptureModule::onCreateFunctionBlock(const StringPtr& id,
                                                             const ComponentPtr& parent,
                                                             const StringPtr& localId,
                                                             const PropertyObjectPtr& config)
{
    if (id == AsamCmpCaptureImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, AsamCmpCaptureImpl>(context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
