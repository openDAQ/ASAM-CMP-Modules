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
#include <asam_cmp/device_status.h>
#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_common_lib/id_manager.h>
#include <asam_cmp_capture_module/encoder_bank.h>
#include <opendaq/context_factory.h>
#include <opendaq/function_block_impl.h>
#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_common_lib/interface_common_fb.h>

namespace daq::asam_cmp_common_lib
{
    class EthernetPcppItf;
}

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

struct InterfaceFbInit
{
    const EncoderBankPtr& encoders;
    ASAM::CMP::DeviceStatus& deviceStatus;
    std::mutex& statusSync;
    const std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf>& ethernetWrapper;
    const bool& allowJumboFrames;
    const StringPtr& selectedDeviceName;
};

class InterfaceFb final : public asam_cmp_common_lib::InterfaceCommonFb
{
public:
    explicit InterfaceFb(const ContextPtr& ctx,
                                    const ComponentPtr& parent,
                                    const StringPtr& localId,
                                    const asam_cmp_common_lib::InterfaceCommonInit& commonInit,
                                    const InterfaceFbInit& internalInit);
    ~InterfaceFb() override = default;
    static FunctionBlockTypePtr CreateType();

private:
    void initProperties();
    void addStreamInternal() override;
    void removeStreamInternal(size_t nInd) override;
    void initStatusPacket();
    void updateInterfaceData();

    void updateInterfaceIdInternal() override;
    void updatePayloadTypeInternal() override;

private:
    std::mutex& statusSync;
    EncoderBankPtr encoders;
    ASAM::CMP::Packet interfaceStatusPacket;
    ASAM::CMP::DeviceStatus& deviceStatus;

    std::unordered_set<uint8_t> streamIdsList;
    std::string vendorDataAsString;
    std::vector<uint8_t> vendorData;

    const std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf>& ethernetWrapper;
    const bool& allowJumboFrames;
    const StringPtr& selectedDeviceName;
};


END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
