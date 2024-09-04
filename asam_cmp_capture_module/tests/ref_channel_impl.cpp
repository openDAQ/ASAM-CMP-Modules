#include <coreobjects/callable_info_factory.h>
#include <coreobjects/coercer_factory.h>
#include <coreobjects/eval_value_factory.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coreobjects/unit_factory.h>
#include <coretypes/procedure_factory.h>
#include <fmt/format.h>
#include <opendaq/custom_log.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/signal_factory.h>
#include "include/ref_channel_impl.h"

#include <asam_cmp_capture_module/dispatch.h>
#include <opendaq/sample_type_traits.h>

namespace daq
{

    InputChannelStubImpl::InputChannelStubImpl(const ContextPtr& context,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const InputChannelStubInit& init)
        : ChannelImpl(FunctionBlockType("ref_channel", fmt::format("AI{}", init.index + 1), ""), context, parent, localId)
        , index(init.index)
        , sampleRate(init.sampleRate)
        , startTime(init.startTime)
        , microSecondsFromEpochToStartTime(init.microSecondsFromEpochToStartTime)
        , lastCollectTime(0)
        , samplesGenerated(0)
        , needsSignalTypeChanged(false)
        , generateFunc(init.generateFunc)
        , isDescriptorsInitialized(false)
        , sampleType(SampleType::Int16)
    {
        initProperties();
        signalTypeChangedInternal();
        packetSizeChangedInternal();
        createSignals();
        buildDefaultDescriptors();
    }

    void InputChannelStubImpl::initDescriptors()
    {
        std::scoped_lock lock(sync);

        isDescriptorsInitialized = true;
        buildSignalDescriptors();
    }

    void InputChannelStubImpl::clearDescriptors()
    {
        buildDefaultDescriptors();
    }

    bool InputChannelStubImpl::getIsDescriptorsInitialized() const
    {
        return isDescriptorsInitialized;
    }

    void InputChannelStubImpl::signalTypeChangedIfNotUpdating(const PropertyValueEventArgsPtr& args)
    {
        if (!args.getIsUpdating())
            signalTypeChanged();
        else
            needsSignalTypeChanged = true;
    }

    void InputChannelStubImpl::initProperties()
    {
        const auto sampleRateProp = FloatPropertyBuilder("SampleRate", 100.0)
                                        .setUnit(Unit("Hz"))
                                        .setMinValue(1.0)
                                        .setMaxValue(1000000.0)
                                        .setSuggestedValues(List<Float>(10.0, 100.0, 1000.0, 10000.0, 100000.0, 1000000.0))
                                        .build();

        objPtr.addProperty(sampleRateProp);
        objPtr.getOnPropertyValueWrite("SampleRate") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
        {
            if (args.getPropertyEventType() == PropertyEventType::Update)
            {
                double sr = args.getValue();
                auto coercedSr = this->coerceSampleRate(sr);
                if (coercedSr != sr)
                    args.setValue(coercedSr);
            }

            signalTypeChangedIfNotUpdating(args);
        };

        const auto resetCounterProp = FunctionProperty("ResetCounter", ProcedureInfo(), EvalValue("$Waveform == 3"));
        objPtr.addProperty(resetCounterProp);
        objPtr.setPropertyValue("ResetCounter", Procedure([this] { this->resetCounter(); }));

        const auto clientSideScalingProp = BoolProperty("ClientSideScaling", False);

        objPtr.addProperty(clientSideScalingProp);
        objPtr.getOnPropertyValueWrite("ClientSideScaling") +=
            [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { signalTypeChangedIfNotUpdating(args); };

        customRange = Range(-10.0, 10.0);
        objPtr.addProperty(StructPropertyBuilder("CustomRange", customRange).build());
        objPtr.getOnPropertyValueWrite("CustomRange") +=
            [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { signalTypeChangedIfNotUpdating(args); };

        objPtr.addProperty(IntPropertyBuilder("Delta", 0).setReadOnly(true).build());
        objPtr.getOnPropertyValueWrite("Delta") +=
            [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { deltaT = args.getValue(); };

        objPtr.addProperty(IntPropertyBuilder("SampleType", 0).build());
        objPtr.getOnPropertyValueWrite("SampleType") +=
            [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { signalTypeChangedIfNotUpdating(args); };

        objPtr.addProperty(BoolPropertyBuilder("FixedPacketSize", False).build());
        objPtr.getOnPropertyValueWrite("FixedPacketSize") +=
            [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { packetSizeChanged(); };

        objPtr.addProperty(IntPropertyBuilder("PacketSize", 1000).setVisible(EvalValue("$FixedPacketSize")).build());
        objPtr.getOnPropertyValueWrite("PacketSize") +=
            [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { packetSizeChanged(); };
    }

    void InputChannelStubImpl::packetSizeChangedInternal()
    {
        fixedPacketSize = objPtr.getPropertyValue("FixedPacketSize");
        packetSize = objPtr.getPropertyValue("PacketSize");
    }

    void InputChannelStubImpl::packetSizeChanged()
    {
        std::scoped_lock lock(sync);

        packetSizeChangedInternal();
    }

    void InputChannelStubImpl::updateSamplesGenerated()
    {
        if (lastCollectTime.count() > 0)
            samplesGenerated = getSamplesSinceStart(lastCollectTime);
    }

    void InputChannelStubImpl::signalTypeChanged()
    {
        std::scoped_lock lock(sync);
        signalTypeChangedInternal();
        buildSignalDescriptors();
        updateSamplesGenerated();
    }

    void InputChannelStubImpl::signalTypeChangedInternal()
    {
        sampleRate = objPtr.getPropertyValue("SampleRate");
        sampleType = objPtr.getPropertyValue("SampleType");
        clientSideScaling = objPtr.getPropertyValue("ClientSideScaling");
        customRange = objPtr.getPropertyValue("CustomRange");
    }

    void InputChannelStubImpl::resetCounter()
    {
        std::scoped_lock lock(sync);
        counter = 0;
    }

    uint64_t InputChannelStubImpl::getSamplesSinceStart(std::chrono::microseconds time) const
    {
        const uint64_t samplesSinceStart =
            static_cast<uint64_t>(std::trunc(static_cast<double>((time - startTime).count()) / 1000000.0 * sampleRate));
        return samplesSinceStart;
    }

    void InputChannelStubImpl::collectSamples(std::chrono::microseconds curTime)
    {
        std::scoped_lock lock(sync);
        if (!isDescriptorsInitialized)
            return;

        const uint64_t samplesSinceStart = getSamplesSinceStart(curTime);
        auto newSamples = samplesSinceStart - samplesGenerated;

        if (newSamples > 0 && valueSignal.getActive())
        {
            if (!fixedPacketSize)
            {
                const auto packetTime = samplesGenerated * deltaT + static_cast<uint64_t>(microSecondsFromEpochToStartTime.count());
                generateSamples(static_cast<int64_t>(packetTime), samplesGenerated, newSamples);
                samplesGenerated = samplesSinceStart;
            }
            else
            {
                while (newSamples >= packetSize)
                {
                    const auto packetTime = samplesGenerated * deltaT + static_cast<uint64_t>(microSecondsFromEpochToStartTime.count());
                    generateSamples(static_cast<int64_t>(packetTime), samplesGenerated, packetSize);

                    samplesGenerated += packetSize;
                    newSamples -= packetSize;
                }
            }
        }

        lastCollectTime = curTime;
    }

    template <SampleType SrcType>
    void generateSamplesInternal(SignalConfigPtr& timeSignal,
                                 SignalConfigPtr& valueSignal,
                                 int64_t curTime,
                                 uint64_t samplesGenerated,
                                 uint64_t newSamples,
                                 const std::function<int()>& generateFunc)
    {
        using SourceType = typename SampleTypeToType<SrcType>::Type;

        const auto domainPacket = DataPacket(timeSignal.getDescriptor(), newSamples, curTime);
        const auto dataPacket = DataPacketWithDomain(domainPacket, valueSignal.getDescriptor(), newSamples);

        auto buffer = static_cast<SourceType*>(dataPacket.getRawData());
        for (uint64_t i = 0; i < newSamples; i++)
            buffer[i] = generateFunc();

        valueSignal.sendPacket(dataPacket);
        timeSignal.sendPacket(domainPacket);
    }

    void generateScaledSamplesInternal(SignalConfigPtr& timeSignal,
                                       SignalConfigPtr& valueSignal,
                                       int64_t curTime,
                                       uint64_t samplesGenerated,
                                       uint64_t newSamples,
                                       const std::function<int()>& generateFunc)
    {
        const auto domainPacket = DataPacket(timeSignal.getDescriptor(), newSamples, curTime);
        const auto dataPacket = DataPacketWithDomain(domainPacket, valueSignal.getDescriptor(), newSamples);

        auto buffer = static_cast<double*>(std::malloc(newSamples * sizeof(double)));
        for (uint64_t i = 0; i < newSamples; i++)
            buffer[i] = generateFunc();

        double f = std::pow(2, 24);
        auto packetBuffer = static_cast<uint32_t*>(dataPacket.getRawData());
        for (size_t i = 0; i < newSamples; i++)
            *packetBuffer++ = static_cast<uint32_t>((buffer[i] + 10.0) / 20.0 * f);

        std::free(static_cast<void*>(buffer));

        valueSignal.sendPacket(dataPacket);
        timeSignal.sendPacket(domainPacket);
    }

    void InputChannelStubImpl::generateSamples(int64_t curTime, uint64_t samplesGenerated, uint64_t newSamples)
    {
        if (clientSideScaling)
        {
            generateScaledSamplesInternal(timeSignal, valueSignal, curTime, samplesGenerated, newSamples, generateFunc);
        }
        else
        {
            SAMPLE_TYPE_DISPATCH(sampleType, generateSamplesInternal, timeSignal, valueSignal, curTime, samplesGenerated, newSamples, generateFunc)
        }

    }

    Int InputChannelStubImpl::getDeltaT(const double sr) const
    {
        const double tickPeriod = getResolution();
        const double samplePeriod = 1.0 / sr;
        const Int newDeltaT = static_cast<Int>(std::round(samplePeriod / tickPeriod));
        objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("Delta", newDeltaT);
        return newDeltaT;
    }

    void InputChannelStubImpl::buildSignalDescriptors()
    {
        if (!isDescriptorsInitialized)
            return;

        const auto valueDescriptorBuilder = DataDescriptorBuilder()
                                         .setSampleType(sampleType)
                                         .setValueRange(customRange)
                                         .setUnit(Unit("t", -1, "test", "count"))
                                         .setName("AI " + std::to_string(index + 1));

        if (clientSideScaling)
        {
            const double scale = 20.0 / std::pow(2, 24);
            constexpr double offset = -10.0;
            valueDescriptorBuilder.setPostScaling(LinearScaling(scale, offset, SampleType::Int32, ScaledSampleType::Float64));
        }

        valueSignal.setDescriptor(valueDescriptorBuilder.build());

        deltaT = getDeltaT(sampleRate);

        const auto timeDescriptor = DataDescriptorBuilder()
                                        .setSampleType(SampleType::Int64)
                                        .setUnit(Unit("s", -1, "seconds", "time"))
                                        .setTickResolution(getResolution())
                                        .setRule(LinearDataRule(deltaT, 0))
                                        .setOrigin(getEpoch())
                                        .setName("Time AI " + std::to_string(index + 1));

        timeSignal.setDescriptor(timeDescriptor.build());
    }

    void InputChannelStubImpl::buildDefaultDescriptors()
    {
        std::scoped_lock lock(sync);

        isDescriptorsInitialized = false;

        timeSignal.setDescriptor(DataDescriptorBuilder().build());
        valueSignal.setDescriptor(DataDescriptorBuilder().build());
    }

    double InputChannelStubImpl::coerceSampleRate(const double wantedSampleRate) const
    {
        const double tickPeriod = getResolution();
        const double samplePeriod = 1.0 / wantedSampleRate;

        const double multiplier = samplePeriod / tickPeriod;

        double roundedMultiplier = std::round(multiplier);

        if (roundedMultiplier < 1.0)
            roundedMultiplier = 1.0;

        const double roundedSamplePeriod = roundedMultiplier * tickPeriod;

        double roundedSampleRate = 1.0 / roundedSamplePeriod;

        if (roundedSampleRate > 1000000)
            roundedSampleRate = 1000000;

        return roundedSampleRate;
    }

    void InputChannelStubImpl::createSignals()
    {
        valueSignal = createAndAddSignal(fmt::format("ai{}", index));
        timeSignal = createAndAddSignal(fmt::format("ai{}_time", index), nullptr, false);

        valueSignal.setDomainSignal(timeSignal);
    }

    void InputChannelStubImpl::globalSampleRateChanged(double newGlobalSampleRate)
    {
        std::scoped_lock lock(sync);

        globalSampleRate = coerceSampleRate(newGlobalSampleRate);
        signalTypeChangedInternal();
        buildSignalDescriptors();
        updateSamplesGenerated();
    }

    std::string InputChannelStubImpl::getEpoch()
    {
        const std::time_t epochTime = std::chrono::system_clock::to_time_t(std::chrono::time_point<std::chrono::system_clock>{});

        char buf[48];
        strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&epochTime));

        return {buf};
    }

    RatioPtr InputChannelStubImpl::getResolution()
    {
        return Ratio(1, 1000000);
    }

    void InputChannelStubImpl::endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating)
    {
        ChannelImpl<IInputChannelStub>::endApplyProperties(propsAndValues, parentUpdating);

        if (needsSignalTypeChanged)
        {
            signalTypeChanged();
            needsSignalTypeChanged = false;
        }
    }

}
