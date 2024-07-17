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
#include <asam_cmp_capture_module/common.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

class CaptureModuleCommonImpl : public FunctionBlock
{
public:
    explicit CaptureModuleCommonImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~CaptureModuleCommonImpl() override = default;

    static FunctionBlockTypePtr CreateType();

protected:
    template <class Impl, typename... Params>
    void addInterfaceWithParams(Params&&... params);
    virtual void addInterfaceInternal() = 0;
    virtual void updateDeviceId();
    virtual void removeInterfaceInternal(size_t nInd);

private:
    void initProperties();

protected:
    AsamCmpInterfaceIdManager interfaceIdManager;
    AsamCmpStreamIdManager streamIdManager;
    uint16_t deviceId{0};
};

template <class Impl, typename... Params>
void CaptureModuleCommonImpl::addInterfaceWithParams(Params&&... params)
{
    auto newId = interfaceIdManager.getFirstUnusedId();
    AsamCmpInterfaceCommonInit init{newId, &interfaceIdManager, &streamIdManager};

    StringPtr fbId = fmt::format("asam_cmp_interface_{}", newId);
    auto newFb = createWithImplementation<IFunctionBlock, Impl>(context, functionBlocks, fbId, init, std::forward<Params>(params)...);
    functionBlocks.addItem(newFb);
    interfaceIdManager.addId(newId);
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
