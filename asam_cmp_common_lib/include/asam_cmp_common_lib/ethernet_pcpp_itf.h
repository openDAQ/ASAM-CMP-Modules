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
#include <asam_cmp_common_lib/ethernet_itf.h>
#include <PcapLiveDeviceList.h>
#include <coreobjects/callable_info_factory.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

class EthernetPcppItf : public EthernetItf<std::function<void(pcpp::RawPacket*, pcpp::PcapLiveDevice*, void*)>>
{
    public:
    virtual ~EthernetPcppItf() = default;
};

END_NAMESPACE_ASAM_CMP_COMMON
