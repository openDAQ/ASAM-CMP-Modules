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
#include <asam_cmp/payload_type.h>
#include <opendaq/context_factory.h>
#include <opendaq/function_block_impl.h>

#include <asam_cmp_common_lib/id_manager.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

struct StreamCommonInit
{
    const uint32_t id;
    ASAM::CMP::PayloadType payloadType;
    const StreamIdManagerPtr streamIdManager;
};

DECLARE_OPENDAQ_INTERFACE(IStreamCommon, IBaseObject)
{
    virtual void setPayloadType(ASAM::CMP::PayloadType type) = 0;
};

template <typename... Interfaces>
class StreamCommonFbImpl : public FunctionBlockImpl<IFunctionBlock, IStreamCommon, Interfaces...>
{
protected:
    using PayloadType = ASAM::CMP::PayloadType;

public:
    explicit StreamCommonFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const StreamCommonInit& init);
    ~StreamCommonFbImpl() override = default;
    static FunctionBlockTypePtr CreateType();

public:  // IStreamCommon
    void setPayloadType(PayloadType type) override;

protected:
    virtual void updateStreamIdInternal();

private:
    void initProperties();

protected:
    uint8_t streamId;
    PayloadType payloadType{0};

    StreamIdManagerPtr streamIdManager;
};

using StreamCommonFb = StreamCommonFbImpl<>;

template <typename... Interfaces>
StreamCommonFbImpl<Interfaces...>::StreamCommonFbImpl(const ContextPtr& ctx,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId,
                                                      const StreamCommonInit& init)
    : FunctionBlockImpl<IFunctionBlock, IStreamCommon, Interfaces...>(CreateType(), ctx, parent, localId)
    , streamIdManager(init.streamIdManager)
    , streamId(init.id)
    , payloadType(init.payloadType)
{
    initProperties();
}

template <typename... Interfaces>
FunctionBlockTypePtr StreamCommonFbImpl<Interfaces...>::CreateType()
{
    return FunctionBlockType("asam_cmp_stream", "AsamCmpStream", "ASAM CMP Stream");
}

template <typename... Interfaces>
void StreamCommonFbImpl<Interfaces...>::setPayloadType(PayloadType type)
{
    payloadType = type;
}

template <typename... Interfaces>
void StreamCommonFbImpl<Interfaces...>::initProperties()
{
    StringPtr propName = "StreamId";
    auto prop = IntPropertyBuilder(propName, streamId)
                    .setMinValue(static_cast<Int>(std::numeric_limits<uint8_t>::min()))
                    .setMaxValue(static_cast<Int>(std::numeric_limits<uint8_t>::max()))
                    .build();
    this->objPtr.addProperty(prop);
    this->objPtr.getOnPropertyValueWrite(propName) +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateStreamIdInternal(); };
}

template <typename... Interfaces>
void StreamCommonFbImpl<Interfaces...>::updateStreamIdInternal()
{
    uint8_t newId = static_cast<Int>(this->objPtr.getPropertyValue("StreamId"));

    if (newId == streamId)
        return;

    streamIdManager->removeId(streamId);
    streamId = newId;
    streamIdManager->addId(streamId);
}

END_NAMESPACE_ASAM_CMP_COMMON
