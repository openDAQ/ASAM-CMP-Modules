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
#include <opendaq/context_factory.h>
#include <opendaq/function_block_impl.h>
#include <asam_cmp_common_lib/network_manager_fb.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

class CaptureModuleFb final : public asam_cmp_common_lib::NetworkManagerFb
{
public:
    explicit CaptureModuleFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const std::shared_ptr<asam_cmp_common_lib::EthernetPcppItf>& ethernetWrapper);
    ~CaptureModuleFb() override = default;

    static FunctionBlockTypePtr CreateType();
    static FunctionBlockPtr create(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);

private:
    void createFbs();
    void networkAdapterChangedInternal() override;
};


END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
