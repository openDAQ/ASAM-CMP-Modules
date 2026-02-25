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
#include <asam_cmp/status.h>
#include <opendaq/function_block_impl.h>

#include <asam_cmp_data_sink/capture_packets_publisher.h>
#include <asam_cmp_data_sink/common.h>
#include <asam_cmp_data_sink/data_packets_publisher.h>
#include <asam_cmp_data_sink/status_handler.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

class DataSinkFb final : public FunctionBlock
{
public:
    explicit DataSinkFb(const ModuleInfoPtr& moduleInfo,
                        const ContextPtr& ctx,
                        const ComponentPtr& parent,
                        const StringPtr& localId,
                        StatusMt statusMt,
                        DataPacketsPublisher& dataPacketsPublisher,
                        CapturePacketsPublisher& capturePacketsPublisher);
    ~DataSinkFb() override = default;

    static FunctionBlockTypePtr CreateType(const ModuleInfoPtr& moduleInfo);

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;

private:
    void initProperties();
    void addCaptureModuleFromStatus(int index);
    void addCaptureModuleEmpty();
    void removeCaptureModule(int fbIndex);

    [[nodiscard]] static StringPtr getFbId(size_t id);
    [[nodiscard]] static StringPtr getFbName(size_t id);

private:
    size_t captureModuleId{0};
    StatusMt status;
    DataPacketsPublisher& dataPacketsPublisher;
    CapturePacketsPublisher& capturePacketsPublisher;
};

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
