#include <asam_cmp_data_sink/data_sink_module_fb.h>
#include <asam_cmp_data_sink/data_sink_module.h>
#include <asam_cmp_data_sink/version.h>
#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

DataSinkModule::DataSinkModule(ContextPtr ctx)
    : Module("ASAM CMP Data Sink Module",
             daq::VersionInfo(ASAM_CMP_DATA_SINK_MAJOR_VERSION, ASAM_CMP_DATA_SINK_MINOR_VERSION, ASAM_CMP_DATA_SINK_PATCH_VERSION),
             std::move(ctx),
             "AsamCmpDataSinkModule")
{
}

DictPtr<IString, IFunctionBlockType> DataSinkModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    auto typeStatistics = DataSinkModuleFb::CreateType();
    types.set(typeStatistics.getId(), typeStatistics);

    return types;
}

FunctionBlockPtr DataSinkModule::onCreateFunctionBlock(const StringPtr& id,
                                                              const ComponentPtr& parent,
                                                              const StringPtr& localId,
                                                              const PropertyObjectPtr& config)
{
    if (id == DataSinkModuleFb::CreateType().getId())
    {
        FunctionBlockPtr fb = DataSinkModuleFb::create(context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
