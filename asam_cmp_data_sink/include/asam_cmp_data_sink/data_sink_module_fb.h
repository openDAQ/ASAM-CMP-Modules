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
#include <PcapLiveDeviceList.h>
#include <asam_cmp/decoder.h>
#include <asam_cmp_common_lib/network_manager_fb.h>

#include <asam_cmp_data_sink/capture_packets_publisher.h>
#include <asam_cmp_data_sink/common.h>
#include <asam_cmp_data_sink/data_packets_publisher.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

class DataSinkModuleFb final : public asam_cmp_common_lib::NetworkManagerFb
{
public:
    explicit DataSinkModuleFb(const ContextPtr& ctx,
                              const ComponentPtr& parent,
                              const StringPtr& localId,
                              const std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf>& ethernetWrapper);
    ~DataSinkModuleFb() override;

    static FunctionBlockTypePtr CreateType();
    static FunctionBlockPtr create(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ErrCode INTERFACE_FUNC remove() override;

private:
    void createFbs();
    void startCapture();
    void stopCapture();
    void onPacketArrives(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie);
    std::vector<std::shared_ptr<ASAM::CMP::Packet>> decode(pcpp::RawPacket* packet);

    void networkAdapterChangedInternal() override;

private:
    bool captureStartedOnThisFb;
    ASAM::CMP::Decoder decoder;

    DataPacketsPublisher dataPacketsPublisher;
    CapturePacketsPublisher capturePacketsPublisher;
};

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
