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
#include <asam_cmp/device_status.h>
#include <opendaq/function_block_impl.h>

#include <asam_cmp_common_lib/id_manager.h>
#include <asam_cmp_common_lib/capture_common_fb.h>
#include <asam_cmp_data_sink/common.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

class CaptureModuleFb final : public asam_cmp_common_lib::CaptureCommonFb
{
public:
    explicit CaptureModuleFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    explicit CaptureModuleFb(const ContextPtr& ctx,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               ASAM::CMP::DeviceStatus&& deviceStatus);
    ~CaptureModuleFb() override = default;

protected:
    void addInterfaceInternal() override;

private:
    void createFbs();

private:
    ASAM::CMP::DeviceStatus deviceStatus;
};

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
