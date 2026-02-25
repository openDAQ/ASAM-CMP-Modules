/*
 * Copyright 2022-2024 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <coretypes/common.h>
#include <opendaq/channel_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <optional>
#include <random>

namespace daq
{

    DECLARE_OPENDAQ_INTERFACE(IInputChannelStub, IBaseObject)
    {
        virtual void collectSamples(std::chrono::microseconds curTime) = 0;
        virtual void globalSampleRateChanged(double globalSampleRate) = 0;
        virtual void initDescriptors() = 0;
        virtual void clearDescriptors() = 0;
        virtual bool getIsDescriptorsInitialized() const = 0;
    };

    struct InputChannelStubInit
    {
        size_t index;
        double sampleRate;
        std::chrono::microseconds startTime;
        std::chrono::microseconds microSecondsFromEpochToStartTime;
        std::function<int()> generateFunc;
    };

    class InputChannelStubImpl final : public ChannelImpl<IInputChannelStub>
    {
    public:
        explicit InputChannelStubImpl(const ContextPtr& context,
                                      const ComponentPtr& parent,
                                      const StringPtr& localId,
                                      const InputChannelStubInit& init);

        // IInputChannelStub
        void collectSamples(std::chrono::microseconds curTime) override;
        void globalSampleRateChanged(double newGlobalSampleRate) override;
        void initDescriptors() override;
        void clearDescriptors() override;
        bool getIsDescriptorsInitialized() const override;
        static std::string getEpoch();
        static RatioPtr getResolution();

    protected:
        void endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating) override;

    private:
        bool clientSideScaling;
        StructPtr customRange;
        double sampleRate;
        size_t index;
        double globalSampleRate;
        uint64_t counter;
        uint64_t deltaT;
        std::chrono::microseconds startTime;
        std::chrono::microseconds microSecondsFromEpochToStartTime;
        std::chrono::microseconds lastCollectTime;
        uint64_t samplesGenerated;

        SampleType sampleType;
        SignalConfigPtr valueSignal;
        SignalConfigPtr timeSignal;
        bool isDescriptorsInitialized;
        bool needsSignalTypeChanged;
        bool fixedPacketSize;
        uint64_t packetSize;
        std::function<int()> generateFunc;
        bool isSuperCounter;

        void initProperties();
        void packetSizeChangedInternal();
        void packetSizeChanged();
        void updateSamplesGenerated();
        void signalTypeChanged();
        void signalTypeChangedInternal();
        void resetCounter();
        uint64_t getSamplesSinceStart(std::chrono::microseconds time) const;
        void createSignals();
        void generateSamples(int64_t curTime, uint64_t samplesGenerated, uint64_t newSamples);
        [[nodiscard]] Int getDeltaT(const double sr) const;
        void buildSignalDescriptors();
        void buildDefaultDescriptors();
        [[nodiscard]] double coerceSampleRate(const double wantedSampleRate) const;
        void signalTypeChangedIfNotUpdating(const PropertyValueEventArgsPtr& args);
    };

}
