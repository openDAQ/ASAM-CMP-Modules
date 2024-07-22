#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <asam_cmp_capture_module/capture_module.h>
#include <asam_cmp_capture_module/version.h>
#include <asam_cmp_capture_module/capture_module_fb.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

CaptureModule::CaptureModule(ContextPtr ctx)
    : Module("ASAM CMP CaptureModule",
             daq::VersionInfo(ASAM_CMP_CAPTURE_MODULE_MAJOR_VERSION, ASAM_CMP_CAPTURE_MODULE_MINOR_VERSION, ASAM_CMP_CAPTURE_MODULE_PATCH_VERSION),
             std::move(ctx),
             "AsamCmpCaptureModule")
{
}

DictPtr<IString, IFunctionBlockType> CaptureModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    auto typeStatistics = CaptureModuleFb::CreateType();
    types.set(typeStatistics.getId(), typeStatistics);

    return types;
}

FunctionBlockPtr CaptureModule::onCreateFunctionBlock(const StringPtr& id,
                                                             const ComponentPtr& parent,
                                                             const StringPtr& localId,
                                                             const PropertyObjectPtr& config)
{
    if (id == CaptureModuleFb::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, CaptureModuleFb>(context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
