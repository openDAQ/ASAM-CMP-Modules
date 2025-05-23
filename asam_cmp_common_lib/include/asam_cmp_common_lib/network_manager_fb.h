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
#include <opendaq/function_block_impl.h>
#include <asam_cmp_common_lib/common.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

class EthernetPcppItf;

class NetworkManagerFb : public FunctionBlock
{
public:
    explicit NetworkManagerFb(const FunctionBlockTypePtr& type,
                              const ContextPtr& ctx,
                              const ComponentPtr& parent,
                              const StringPtr& localId,
                              const std::shared_ptr<EthernetPcppItf>& ethernetWrapper);
    ~NetworkManagerFb() override;

private:
    void initProperties();
    void addNetworkAdaptersProperty();

protected:
    virtual void networkAdapterChangedInternal();

protected:
    std::shared_ptr<EthernetPcppItf> ethernetWrapper;
    StringPtr selectedEthernetDeviceName;
};

END_NAMESPACE_ASAM_CMP_COMMON
