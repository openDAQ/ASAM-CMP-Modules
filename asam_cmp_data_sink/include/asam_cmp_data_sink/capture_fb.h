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
#include <asam_cmp/device_status.h>
#include <opendaq/function_block_impl.h>

#include <asam_cmp_common_lib/capture_common_fb.h>
#include <asam_cmp_common_lib/id_manager.h>
#include <asam_cmp_data_sink/asam_cmp_packets_subscriber.h>
#include <asam_cmp_data_sink/capture_packets_publisher.h>
#include <asam_cmp_data_sink/common.h>
#include <asam_cmp_data_sink/data_packets_publisher.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

class CaptureFb final : public asam_cmp_common_lib::CaptureCommonFbImpl<IAsamCmpPacketsSubscriber>
{
public:
    explicit CaptureFb(const ModuleInfoPtr& moduleInfo,
                       const ContextPtr& ctx,
                       const ComponentPtr& parent,
                       const StringPtr& localId,
                       DataPacketsPublisher& publisher,
                       CapturePacketsPublisher& capturePacketsPublisher);
    explicit CaptureFb(const ModuleInfoPtr& moduleInfo,
                       const ContextPtr& ctx,
                       const ComponentPtr& parent,
                       const StringPtr& localId,
                       DataPacketsPublisher& publisher,
                       CapturePacketsPublisher& capturePacketsPublisher,
                       ASAM::CMP::DeviceStatus&& deviceStatus);
    ~CaptureFb() override = default;

    // IAsamCmpPacketsSubscriber
    void receive(const std::shared_ptr<ASAM::CMP::Packet>& packet) override;
    void receive(const std::vector<std::shared_ptr<ASAM::CMP::Packet>>& packets) override{};

protected:
    void updateDeviceIdInternal() override;
    void addInterfaceInternal() override;
    void removeInterfaceInternal(size_t nInd) override;

private:
    void setProperties();
    void setDeviceInfoProperties(const ASAM::CMP::Packet& packet);
    void createFbs();

private:
    ASAM::CMP::DeviceStatus deviceStatus;
    DataPacketsPublisher& dataPacketsPublisher;
    CapturePacketsPublisher& capturePacketsPublisher;
};

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
