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

#include <opendaq/context_factory.h>
#include <opendaq/function_block_impl.h>

#include <asam_cmp/payload_type.h>
#include <asam_cmp_common_lib/id_manager.h>
#include <asam_cmp_common_lib/stream_common_fb_impl.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

struct InterfaceCommonInit
{
    const uint32_t id;
    InterfaceIdManagerPtr interfaceIdManager;
};

class InterfaceCommonFb : public FunctionBlock
{
private:
    using PayloadType = ASAM::CMP::PayloadType;

public:
    explicit InterfaceCommonFb(const ContextPtr& ctx,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               const InterfaceCommonInit& init);
    ~InterfaceCommonFb() override = default;
    static FunctionBlockTypePtr CreateType();

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;

protected:
    template <class Impl, typename... Params>
    FunctionBlockPtr addStreamWithParams(uint8_t streamId, Params&&... params);
    virtual void updateInterfaceIdInternal();
    virtual void updatePayloadTypeInternal();
    virtual void addStreamInternal() = 0;
    virtual void removeStreamInternal(size_t nInd);

    daq::ErrCode INTERFACE_FUNC beginUpdate() override;
    void endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating) override;
    virtual void propertyChanged();
    void propertyChangedIfNotUpdating();

    [[nodiscard]] uint32_t indexToPayloadType(Int index) const;
    [[nodiscard]] Int payloadTypeToIndex(PayloadType type) const;
    [[nodiscard]] size_t maxPayloadIndex() const;

private:
    void initProperties();
    void addStream();
    void removeStream(size_t nInd);

protected:
    InterfaceIdManagerPtr interfaceIdManager;
    StreamIdManager streamIdManager;
    uint32_t interfaceId;
    PayloadType payloadType;

    std::atomic_bool isUpdating;
    std::atomic_bool needsPropertyChanged;

private:
    size_t createdStreams;
};

template <class Impl, typename... Params>
FunctionBlockPtr InterfaceCommonFb::addStreamWithParams(uint8_t streamId, Params&&... params)
{
    if (isUpdating)
        throw std::runtime_error("Adding streams is disabled during update");

    StreamCommonInit init{streamId, payloadType, &streamIdManager};

    StringPtr fbName = fmt::format("Stream {}", createdStreams);
    StringPtr fbId = fmt::format("Stream_{}", createdStreams++);
    auto newFb = createWithImplementation<IFunctionBlock, Impl>(context, functionBlocks, fbId, init, std::forward<Params>(params)...);
    newFb.setName(fbName);
    streamIdManager.addId(streamId);
    this->addNestedFunctionBlock(newFb);
    return newFb;
}

END_NAMESPACE_ASAM_CMP_COMMON
