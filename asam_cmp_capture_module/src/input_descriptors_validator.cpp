#include <asam_cmp_capture_module/input_descriptors_validator.h>
#include <opendaq/dimension_factory.h>
#include <unordered_set>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

namespace
{

    bool checkDimension(const DimensionPtr& first, const DimensionPtr& second)
    {
        if (first.getSize() != second.getSize())
            return false;

        if (first.getUnit() != second.getUnit())
            return false;

        if (first.getRule() != second.getRule())
            return false;

        return true;
    }

    bool checkDataDescriptor(const DataDescriptorPtr& inputDataDescriptor, const DataDescriptorPtr& referenceDataDescriptor)
    {
        if (!inputDataDescriptor.assigned())
            return false;

        if (inputDataDescriptor.getSampleType() != referenceDataDescriptor.getSampleType())
            return false;

        if (inputDataDescriptor.getUnit() != referenceDataDescriptor.getUnit())
            return false;

        if (inputDataDescriptor.getRule() != referenceDataDescriptor.getRule())
            return false;

        const auto dimensions = inputDataDescriptor.getDimensions();
        const auto refDimensions = referenceDataDescriptor.getDimensions();

        if (dimensions.getCount() != refDimensions.getCount())
            return false;

        for (std::size_t i = 0; i != dimensions.getCount(); i++)
        {
            if (!checkDimension(dimensions.getItemAt(i), refDimensions.getItemAt(i)))
                return false;
        }
        return true;
    }

    DataDescriptorPtr buildCanStructureReference()
    {
        const auto arbIdDescriptor = DataDescriptorBuilder().setName("ArbId").setSampleType(SampleType::Int32).build();

        const auto lengthDescriptor = DataDescriptorBuilder().setName("Length").setSampleType(SampleType::Int8).build();

        const auto dataDescriptor =
            DataDescriptorBuilder()
                .setName("Data")
                .setSampleType(SampleType::UInt8)
                .setDimensions(List<IDimension>(DimensionBuilder().setRule(LinearDimensionRule(0, 1, 64)).setName("Dimension").build()))
                .build();

        const auto canMsgDescriptor = DataDescriptorBuilder()
                                          .setSampleType(SampleType::Struct)
                                          .setStructFields(List<IDataDescriptor>(arbIdDescriptor, lengthDescriptor, dataDescriptor))
                                          .setName("CAN")
                                          .build();
        return canMsgDescriptor;
    }

    DataDescriptorPtr canStructureReference = buildCanStructureReference();

    bool validateStructureSampleType(DataDescriptorPtr inputDataDescriptor, DataDescriptorPtr referenceDataDescriptor)
    {
        auto inputSampleType = inputDataDescriptor.getSampleType();
        if (inputSampleType != SampleType::Struct)
            throw std::runtime_error("Invalid sample type");

        if (!checkDataDescriptor(inputDataDescriptor, referenceDataDescriptor))
            return false;

        auto inputDataDescriptorStruct = inputDataDescriptor.getStructFields();
        auto referenceDataDescriptorStruct = referenceDataDescriptor.getStructFields();
        if (inputDataDescriptorStruct.getCount() != referenceDataDescriptorStruct.getCount())
            return false;

        for (int i = 0; i < inputDataDescriptorStruct.getCount(); ++i)
        {
            if (!checkDataDescriptor(inputDataDescriptorStruct.getItemAt(i), referenceDataDescriptorStruct.getItemAt(i)))
                return false;
        }

        return true;
    }
}

bool validateInputDescriptor(DataDescriptorPtr inputDataDescriptor, const ASAM::CMP::PayloadType& type)
{
    switch (type.getRawPayloadType())
    {
        case uint8_t(ASAM::CMP::PayloadType::can):
            return validateStructureSampleType(inputDataDescriptor, canStructureReference);
        case uint8_t(ASAM::CMP::PayloadType::canFd):
            return validateStructureSampleType(inputDataDescriptor, canStructureReference);
        default:
            return false;
    }
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
