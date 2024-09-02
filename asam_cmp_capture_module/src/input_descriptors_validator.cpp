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

    bool validateAnalogSampleType(DataDescriptorPtr inputDataDescriptor)
    {
        if (inputDataDescriptor.getSampleType() == SampleType::Struct)
            return false;  // throw std::runtime_error("Struct sample type is not allowed");

        if (inputDataDescriptor.getDimensions().getCount() > 0)
            return false;  // throw std::runtime_error("Arrays not supported");

        if (!inputDataDescriptor.getRule().assigned() || inputDataDescriptor.getRule().getType() != DataRuleType::Explicit)
            return false;  // throw std::runtime_error("Only explicit data rule is used");

        if (!hasCorrectPostScaling(inputDataDescriptor.getPostScaling()))
        {
            if (!hasCorrectSampleType(inputDataDescriptor.getSampleType()) && !hasCorrectValueRange(inputDataDescriptor.getValueRange()))
                return false;  // throw std::runtime_error("MinMax range should be assigned");
        }

        return true;
    }
}

bool hasCorrectValueRange(const RangePtr& range)
{
    if (!range.assigned())
        return false;  // throw std::runtime_error("MinMax range should be assigned");

    if (!range.getLowValue().assigned() || !range.getHighValue().assigned())
        return false;  // throw std::runtime_error("MinMax range should be assigned");

    return true;
}

bool hasCorrectSampleType(const SampleType& sampleType)
{
    return sampleType == SampleType::Int16 || sampleType == SampleType::Int32;
}

bool hasCorrectPostScaling(const ScalingPtr& postScaling)
{
    if (!postScaling.assigned())
        return false;

    auto postScalingParams = postScaling.getParameters();
    return hasCorrectSampleType(postScaling.getInputSampleType()) && postScalingParams.hasKey("offset") &&
           postScalingParams.hasKey("scale");
}

bool validateInputDescriptor(DataDescriptorPtr inputDataDescriptor, const ASAM::CMP::PayloadType& type)
{
    switch (type.getType())
    {
        case ASAM::CMP::PayloadType::can:
            return validateStructureSampleType(inputDataDescriptor, canStructureReference);
        case ASAM::CMP::PayloadType::canFd:
            return validateStructureSampleType(inputDataDescriptor, canStructureReference);
        case ASAM::CMP::PayloadType::analog:
            return validateAnalogSampleType(inputDataDescriptor);
        default:
            return false;
    }
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
