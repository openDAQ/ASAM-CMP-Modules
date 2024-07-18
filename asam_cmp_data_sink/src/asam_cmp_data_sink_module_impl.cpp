#include <asam_cmp_data_sink/asam_cmp_data_sink_module_fb_impl.h>
#include <asam_cmp_data_sink/asam_cmp_data_sink_module_impl.h>
#include <asam_cmp_data_sink/version.h>
#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

AsamCmpDataSinkModule::AsamCmpDataSinkModule(ContextPtr ctx)
    : Module("ASAM CMP Data Sink Module",
             daq::VersionInfo(ASAM_CMP_DATA_SINK_MAJOR_VERSION, ASAM_CMP_DATA_SINK_MINOR_VERSION, ASAM_CMP_DATA_SINK_PATCH_VERSION),
             std::move(ctx),
             "AsamCmpDataSinkModule")
{
}

DictPtr<IString, IFunctionBlockType> AsamCmpDataSinkModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    auto typeStatistics = AsamCmpDataSinkModuleFbImpl::CreateType();
    types.set(typeStatistics.getId(), typeStatistics);

    return types;
}

FunctionBlockPtr AsamCmpDataSinkModule::onCreateFunctionBlock(const StringPtr& id,
                                                              const ComponentPtr& parent,
                                                              const StringPtr& localId,
                                                              const PropertyObjectPtr& config)
{
    if (id == AsamCmpDataSinkModuleFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, AsamCmpDataSinkModuleFbImpl>(context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
