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

#include <asam_cmp_common_lib/common.h>
#include <asam_cmp_common_lib/id_manager.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

class CaptureCommonFb : public FunctionBlock
{
public:
    explicit CaptureCommonFb(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~CaptureCommonFb() override = default;

    static FunctionBlockTypePtr CreateType();

protected:
    template <class Impl, typename... Params>
    FunctionBlockPtr addInterfaceWithParams(uint32_t interfaceId, Params&&... params);
    virtual void addInterfaceInternal() = 0;
    virtual void updateDeviceId();
    virtual void removeInterfaceInternal(size_t nInd);

    daq::ErrCode INTERFACE_FUNC beginUpdate() override;
    void endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating) override;
    virtual void propertyChanged();
    void propertyChangedIfNotUpdating();

private:
    void initProperties();
    size_t createdInterfaces;

protected:
    InterfaceIdManager interfaceIdManager;
    StreamIdManager streamIdManager;
    uint16_t deviceId{0};

    std::atomic_bool isUpdating;
    std::atomic_bool needsPropertyChanged;
};

template <class Impl, typename... Params>
FunctionBlockPtr CaptureCommonFb::addInterfaceWithParams(uint32_t interfaceId, Params&&... params)
{
    if (isUpdating)
        throw std::runtime_error("Adding interfaces is disabled during update");
    InterfaceCommonInit init{interfaceId, &interfaceIdManager, &streamIdManager};

    StringPtr fbId = fmt::format("asam_cmp_interface_{}", createdInterfaces++);
    auto newFb = createWithImplementation<IFunctionBlock, Impl>(context, functionBlocks, fbId, init, std::forward<Params>(params)...);
    functionBlocks.addItem(newFb);
    interfaceIdManager.addId(interfaceId);

    return newFb;
}

END_NAMESPACE_ASAM_CMP_COMMON
