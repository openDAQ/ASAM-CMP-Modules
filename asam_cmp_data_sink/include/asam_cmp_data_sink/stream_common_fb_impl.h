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
#include <asam_cmp/payload_type.h>
#include <asam_cmp_data_sink/id_manager.h>
#include <asam_cmp_data_sink/common.h>
#include <opendaq/context_factory.h>
#include <opendaq/function_block_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

struct AsamCmpStreamCommonInit
{
    const uint32_t id;
    const ASAM::CMP::PayloadType& payloadType;
    const AsamCmpStreamIdManagerPtr streamIdManager;
};

class StreamCommonFbImpl : public FunctionBlock
{
public:
    explicit StreamCommonFbImpl(const ContextPtr& ctx,
                                const ComponentPtr& parent,
                                const StringPtr& localId,
                                const AsamCmpStreamCommonInit& init);
    ~StreamCommonFbImpl() override = default;
    static FunctionBlockTypePtr CreateType();

protected:
    virtual void updateStreamIdInternal();

private:
    void initProperties();

protected:
    uint32_t id;

private:
    AsamCmpStreamIdManagerPtr streamIdManager;
    const ASAM::CMP::PayloadType& payloadType;
};

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
