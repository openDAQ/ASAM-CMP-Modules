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
#include <asam_cmp_capture_module/common.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <functional>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

template <typename OnPacketReceivedCallbackType>
class AsamCmpEthernetItf
{
public:
    virtual ~AsamCmpEthernetItf() = default;
    virtual ListPtr<StringPtr> getEthernatDevicesNamesList() = 0;
    virtual ListPtr<StringPtr> getEthernatDevicesDescriptionsList() = 0;
    virtual void sendPacket(const StringPtr& deviceName, const std::vector<uint8_t>& data) = 0;
    virtual void startCapture(const StringPtr& deviceName, OnPacketReceivedCallbackType packetReceivedCb) = 0;
    virtual void stopCapture(const StringPtr& deviceName) = 0;
};

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
