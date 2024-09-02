#pragma once
#include <asam_cmp_capture_module/common.h>
#include <opendaq/data_descriptor_factory.h>
#include <asam_cmp/payload_type.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

bool hasCorrectSampleType(const SampleType& sampleType);
bool hasCorrectPostScaling(const ScalingPtr& postScaling);
bool hasCorrectValueRange(const RangePtr& range);
bool validateInputDescriptor(DataDescriptorPtr inputDataDescriptor, const ASAM::CMP::PayloadType& type);

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
