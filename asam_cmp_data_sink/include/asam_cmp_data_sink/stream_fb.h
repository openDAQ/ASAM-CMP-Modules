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
#include <asam_cmp/analog_payload.h>
#include <asam_cmp/can_payload.h>
#include <asam_cmp/ethernet_payload.h>
#include <asam_cmp/packet.h>
#include <opendaq/packet_factory.h>

#include <asam_cmp_common_lib/stream_common_fb_impl.h>
#include <asam_cmp_data_sink/asam_cmp_packets_subscriber.h>
#include <asam_cmp_data_sink/common.h>
#include <asam_cmp_data_sink/data_packets_publisher.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

#pragma pack(push, 1)
struct CANData
{
    uint32_t arbId;
    uint8_t length;
    uint8_t data[64];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct EthernetData
{
    uint16_t flags;
    uint16_t reserved;
    uint16_t length;
};
#pragma pack(pop)


class StreamFb final : public asam_cmp_common_lib::StreamCommonFbImpl<IAsamCmpPacketsSubscriber>
{
private:
    using Packet = ASAM::CMP::Packet;
    using CanPayload = ASAM::CMP::CanPayload;
    using EthernetPayload = ASAM::CMP::EthernetPayload;
    using AnalogPayload = ASAM::CMP::AnalogPayload;

public:
    explicit StreamFb(const ContextPtr& ctx,
                      const ComponentPtr& parent,
                      const StringPtr& localId,
                      const asam_cmp_common_lib::StreamCommonInit& init,
                      DataPacketsPublisher& publisher,
                      const uint16_t& deviceId,
                      const uint32_t& interfaceId);
    ~StreamFb() override = default;

protected:
    // IStreamCommon
    void setPayloadType(PayloadType type) override;

    // IAsamCmpPacketsSubscriber
    void receive(const std::shared_ptr<Packet>& packet) override;
    void receive(const std::vector<std::shared_ptr<Packet>>& packets) override;

    void updateStreamIdInternal() override;

private:
    void createSignals();
    void buildDataDescriptor();
    void buildCanDescriptor();
    void buildEthernetDescriptor();
    void buildAnalogDescriptor(const AnalogPayload& payload);
    void buildAsyncDomainDescriptor();
    void buildSyncDomainDescriptor(const float sampleInterval);
    void processCanData(const std::vector<std::shared_ptr<Packet>>& packets);
    void processEthernetData(const std::vector<std::shared_ptr<Packet>>& packets);
    void processSyncData(const std::shared_ptr<Packet>& packet);
    bool domainChanged(const AnalogPayload& payload);
    bool dataChanged(const AnalogPayload& payload);

    [[nodiscard]] StringPtr getEpoch() const;
    [[nodiscard]] RatioPtr getResolution();
    [[nodiscard]] Int getDeltaT(const float sampleInterval);
    [[nodiscard]] UnitPtr asamCmpToOpenDaqUnit(AnalogPayload::Unit asamCmpUnit);

private:
    const uint16_t& deviceId;
    const uint32_t& interfaceId;
    DataPacketsPublisher& publisher;

    SignalConfigPtr dataSignal;
    SignalConfigPtr domainSignal;
    bool updateDescriptors{false};
    AnalogPayload::Header analogHeader{};
};

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
