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

namespace daq
{

    InputChannelStubImpl::InputChannelStubImpl(const ContextPtr& context,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const InputChannelStubInit& init)
        : ChannelImpl(FunctionBlockType("ref_channel", fmt::format("AI{}", init.index + 1), ""), context, parent, localId)
        , index(init.index)
        , sampleRate(init.sampleRate)
        , counter(0)
        , startTime(init.startTime)
        , microSecondsFromEpochToStartTime(init.microSecondsFromEpochToStartTime)
        , lastCollectTime(0)
        , samplesGenerated(0)
        , needsSignalTypeChanged(false)
        , generateFunc(init.generateFunc)
        , isSuperCounter(false)
        , isDescriptorsInitialized(false)
    {
        initProperties();
        signalTypeChangedInternal();
        packetSizeChangedInternal();
        resetCounter();
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

        const auto defaultCustomRange = Range(-10.0, 10.0);
        objPtr.addProperty(StructPropertyBuilder("CustomRange", defaultCustomRange).build());
        objPtr.getOnPropertyValueWrite("CustomRange") +=
            [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { signalTypeChangedIfNotUpdating(args); };

        objPtr.addProperty(BoolPropertyBuilder("FixedPacketSize", False).build());
        objPtr.getOnPropertyValueWrite("FixedPacketSize") +=
            [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { packetSizeChanged(); };

        objPtr.addProperty(IntPropertyBuilder("PacketSize", 1000).setVisible(EvalValue("$FixedPacketSize")).build());
        objPtr.getOnPropertyValueWrite("PacketSize") +=
            [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { packetSizeChanged(); };

        objPtr.addProperty(BoolProperty("HasZeroPulse", False));
        objPtr.getOnPropertyValueWrite("HasZeroPulse") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
        {
            std::scoped_lock lock(sync);
            buildSignalDescriptors();
        };

        objPtr.addProperty(IntProperty("SensorMode", 0));
        objPtr.getOnPropertyValueWrite("SensorMode") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
        {
            std::scoped_lock lock(sync);
            buildSignalDescriptors();
        };

        objPtr.addProperty(IntPropertyBuilder("Delta", 0).setReadOnly(true).build());
        objPtr.getOnPropertyValueWrite("Delta") +=
            [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { deltaT = args.getValue(); };

        objPtr.addProperty(FloatPropertyBuilder("EdgeBaseClock", 100e6).build());
        objPtr.getOnPropertyValueWrite("EdgeBaseClock") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
        {
            std::scoped_lock lock(sync);
            buildSignalDescriptors();
        };

        objPtr.addProperty(StringPropertyBuilder("CounterId", "CounterTest").build());
        objPtr.getOnPropertyValueWrite("CounterId") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
        {
            std::scoped_lock lock(sync);
            buildSignalDescriptors();
        };

        objPtr.addProperty(StringPropertyBuilder("CounterChannelType", "DefaultName").build());
        objPtr.getOnPropertyValueWrite("CounterChannelType") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
        {
            std::scoped_lock lock(sync);
            isSuperCounter = (args.getValue() == "RawSuperCounter");
            buildSignalDescriptors();
        };
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

    void InputChannelStubImpl::generateSamples(int64_t curTime, uint64_t samplesGenerated, uint64_t newSamples)
    {
        const auto domainPacket = DataPacket(timeSignal.getDescriptor(), newSamples, curTime);
        const auto dataPacket = DataPacketWithDomain(domainPacket, valueSignal.getDescriptor(), newSamples);

        int* buffer;

        buffer = static_cast<int*>(dataPacket.getRawData());

        for (uint64_t i = 0; i < newSamples; i++)
        {
            if (isSuperCounter)
                buffer[i << 1] = buffer[(i << 1) + 1] = generateFunc();
            else
                buffer[i] = generateFunc();
        }

        valueSignal.sendPacket(dataPacket);
        timeSignal.sendPacket(domainPacket);
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
        auto metadataBuilder = [&]()
        {
            daq::DictPtr<IString, IString> ptr = Dict<IString, IString>();
            ptr.set("CounterId", objPtr.getPropertyValue("CounterId"));
            ptr.set("CounterChannelType", objPtr.getPropertyValue("CounterChannelType"));
            ptr.set("EdgeBaseClock", objPtr.getPropertyValue("EdgeBaseClock").toString());
            ptr.set("HasZeroPulse", objPtr.getPropertyValue("HasZeroPulse").toString());
            ptr.set("SensorMode", objPtr.getPropertyValue("SensorMode").toString());

            return ptr;
        };

        const auto valueDescriptorBuilder = DataDescriptorBuilder()
                                                .setSampleType(SampleType::Int32)
                                                .setUnit(Unit("t", -1, "test", "count"))
                                                .setMetadata(metadataBuilder())
                                                .setName("AI " + std::to_string(index + 1));

        auto superCounterDescriptorBuilder = [&]() -> DataDescriptorBuilderPtr
        {
            const auto rawCountDescriptor = DataDescriptorBuilder().setName("RawCount").setSampleType(SampleType::Int32).build();

            const auto rawEdgeSeparationDescriptor =
                DataDescriptorBuilder().setName("RawEdgeSeparation").setSampleType(SampleType::Int32).build();

            const auto superCounterDescriptor = DataDescriptorBuilder()
                                                    .setSampleType(SampleType::Struct)
                                                    .setStructFields(List<IDataDescriptor>(rawCountDescriptor, rawEdgeSeparationDescriptor))
                                                    .setName("RawSuperCounter")
                                                    .setMetadata(metadataBuilder());
            return superCounterDescriptor;
        };

        valueSignal.setDescriptor((isSuperCounter ? superCounterDescriptorBuilder() : valueDescriptorBuilder).build());

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
