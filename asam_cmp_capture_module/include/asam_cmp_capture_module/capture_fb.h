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
#include <asam_cmp_common_lib/id_manager.h>
#include <asam_cmp/device_status.h>
#include <asam_cmp_capture_module/encoder_bank.h>
#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_common_lib/capture_common_fb.h>
#include <asam_cmp/capture_module_payload.h>

namespace daq::asam_cmp_common_lib
{
    class EthernetPcppItf;
}

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

struct CaptureFbInit
{
    const std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf>& ethernetWrapper;
    const StringPtr& selectedDeviceName;
};

class CaptureFb final : public daq::asam_cmp_common_lib::CaptureCommonFb
{
public:
    explicit CaptureFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const CaptureFbInit& init);
    ~CaptureFb() override;

    static FunctionBlockTypePtr CreateType();

private:
    void initProperties();
    void initEncoders();
    void initStatusPacket();
    void updateCaptureData();

    void addInterfaceInternal() override;

    void statusLoop();
    void startStatusLoop();
    void stopStatusLoop();
    ASAM::CMP::DataContext createEncoderDataContext() const;

private:
    const bool allowJumboFrames;
    EncoderBank encoders;
    ASAM::CMP::Packet captureStatusPacket;
    ASAM::CMP::DeviceStatus captureStatus;
    StringPtr deviceDescription;
    StringPtr serialNumber;
    StringPtr hardwareVersion;
    StringPtr softwareVersion;
    std::string vendorDataAsString;
    std::vector<uint8_t> vendorData;

    std::thread statusThread;
    std::mutex statusSync;
    std::condition_variable cv;
    const size_t sendingSyncLoopTime{1000};
    bool stopStatusSending;
    std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf> ethernetWrapper;
    const StringPtr selectedEthernetDeviceName;
};

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
