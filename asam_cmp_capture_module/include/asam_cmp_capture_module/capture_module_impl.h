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
#include <opendaq/function_block_impl.h>
#include <asam_cmp_capture_module/asam_cmp_id_manager.h>
#include <asam_cmp_capture_module/asam_cmp_encoder_bank.h>
#include <asam_cmp_capture_module/common.h>
#include <asam_cmp_capture_module/asam_cmp_ethernet_pcpp_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

class CaptureModuleImpl final : public FunctionBlock
{
public:
    explicit CaptureModuleImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~CaptureModuleImpl() override = default;

    static FunctionBlockTypePtr CreateType();

private:
    void initProperties();
    void initEncoders();

    void addInterfaceInternal();
    void removeInterfaceInternal(size_t nInd);
    void updateDeviceId();


private:
    AsamCmpEncoderBank encoders;
    AsamCmpInterfaceIdManager interfaceIdManager;
    AsamCmpStreamIdManager streamIdManager;
    uint16_t deviceId;

    AsamCmpEthernetPcppImpl ethernetImpl;
};

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
