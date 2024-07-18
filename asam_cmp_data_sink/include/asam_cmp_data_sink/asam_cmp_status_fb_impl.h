/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <asam_cmp/status.h>
#include <opendaq/function_block_impl.h>

#include <asam_cmp_data_sink/asam_cmp_status_handler.h>
#include <asam_cmp_data_sink/common.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

class AsamCmpStatusFbImpl final : public FunctionBlockImpl<IFunctionBlock, IAsamCmpStatusHandler>
{
public:
    explicit AsamCmpStatusFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~AsamCmpStatusFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

public:
    void processStatusPacket(const std::shared_ptr<ASAM::CMP::Packet>& packet) override;
    StatusMt getStatusMt() const override;

private:
    void initProperties();
    void clear();

private:
    mutable std::mutex stMutex;
    ASAM::CMP::Status status;
};

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
