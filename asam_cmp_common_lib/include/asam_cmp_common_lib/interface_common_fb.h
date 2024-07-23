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

#include <opendaq/context_factory.h>
#include <opendaq/function_block_impl.h>

#include <asam_cmp/encoder.h>
#include <asam_cmp_common_lib/id_manager.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

struct InterfaceCommonInit
{
    const uint32_t id;
    InterfaceIdManagerPtr interfaceIdManager;
    StreamIdManagerPtr streamIdManager;
};

class InterfaceCommonFb : public FunctionBlock
{
public:
    explicit InterfaceCommonFb(const ContextPtr& ctx,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               const InterfaceCommonInit& init);
    ~InterfaceCommonFb() override = default;
    static FunctionBlockTypePtr CreateType();

protected:
    template <class Impl, typename... Params>
    FunctionBlockPtr addStreamWithParams(uint8_t streamId, Params&&... params);
    virtual void updateInterfaceIdInternal();
    virtual void updatePayloadTypeInternal();
    virtual void addStreamInternal() = 0;
    virtual void removeStreamInternal(size_t nInd);

private:
    void initProperties();

protected:
    InterfaceIdManagerPtr interfaceIdManager;
    StreamIdManagerPtr streamIdManager;
    uint32_t id;
    ASAM::CMP::PayloadType payloadType;

    // temporary solution once not full list of types is immplemented
    inline static std::map<int, int> payloadTypeToAsamPayloadType = {{0, 0}, {1, 1}, {2, 2}, {3, 7}};
    inline static std::map<int, int> asamPayloadTypeToPayloadType = {{0, 0}, {1, 1}, {2, 2}, {7, 3}};
};

template <class Impl, typename... Params>
FunctionBlockPtr InterfaceCommonFb::addStreamWithParams(uint8_t streamId, Params&&... params)
{
    StreamCommonInit init{streamId, payloadType, streamIdManager};

    StringPtr fbId = fmt::format("asam_cmp_stream_{}", streamId);
    auto newFb = createWithImplementation<IFunctionBlock, Impl>(context, functionBlocks, fbId, init, std::forward<Params>(params)...);
    functionBlocks.addItem(newFb);
    streamIdManager->addId(streamId);

    return newFb;
}

END_NAMESPACE_ASAM_CMP_COMMON
