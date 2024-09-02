/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <asam_cmp/encoder.h>
#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_common_lib/id_manager.h>
#include <asam_cmp_common_lib/stream_common_fb_impl.h>
#include <asam_cmp_capture_module/encoder_bank.h>
#include <opendaq/context_factory.h>
#include <opendaq/function_block_impl.h>
#include <asam_cmp/payload_type.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ptr.h>


namespace daq::asam_cmp_common_lib
{
    class EthernetPcppItf;
}

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

struct StreamInit
{
    std::unordered_set<uint8_t>& streamIdsList;
    std::mutex& statusSync;
    const uint32_t& interfaceId;
    const std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf>& ethernetWrapper;
    const bool& allowJumboFrames;
    const EncoderBankPtr encoderBank;
    std::function<void()> parentInterfaceUpdater;
};

class StreamFb final : public asam_cmp_common_lib::StreamCommonFb
{
public:
    explicit StreamFb(const ContextPtr& ctx,
                      const ComponentPtr& parent,
                      const StringPtr& localId,
                      const asam_cmp_common_lib::StreamCommonInit& init,
                      const StreamInit& internalInit);
    ~StreamFb() override = default;
private:
    void setPayloadType(ASAM::CMP::PayloadType type) override;

    void createInputPort();
    void updateStreamIdInternal() override;

    void initProperties();

    void initStatuses();
    void setInputStatus(const StringPtr& value);
    void onAnalogSignalConnected();
    void configureScaledAnalogSignal();
    void configureMinMaxAnalogSignal();
    void onAnalogSignalDisconnected();

    void onPacketReceived(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;
    void processSignalDescriptorChanged(DataDescriptorPtr inputDataDescriptor, DataDescriptorPtr inputDomainDataDescriptor);
    void configure();

    void processDataPacket(const DataPacketPtr& packet);
    void processCanPacket(const DataPacketPtr& packet);
    void processCanFdPacket(const DataPacketPtr& packet);
    void processAnalogPacket(const DataPacketPtr& packet);

    void processEventPacket(const EventPacketPtr& packet);
    ASAM::CMP::DataContext createEncoderDataContext() const;

private:
    const uint32_t& interfaceId;
    std::unordered_set<uint8_t>& streamIdsList;
    std::mutex& statusSync;
    const EncoderBankPtr encoders;
    std::function<void()> parentInterfaceUpdater;

    InputPortPtr inputPort;
    DataDescriptorPtr inputDataDescriptor;
    DataDescriptorPtr inputDomainDataDescriptor;
    SampleType inputSampleType;

    std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf> ethernetWrapper;
    const bool allowJumboFrames;
    ASAM::CMP::DataContext dataContext;
    const StringPtr& selectedDeviceName;

    //for analog data
    double analogDataDeltaTime;
    double analogDataMin;
    double analogDataMax;
    double analogDataScale;
    double analogDataOffset;
    size_t analogDataSampleDt = 32;
    bool analogDataHasInternalPostScaling;
};

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
