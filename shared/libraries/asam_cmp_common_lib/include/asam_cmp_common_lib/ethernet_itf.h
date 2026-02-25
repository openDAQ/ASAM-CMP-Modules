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
#include <asam_cmp_common_lib/common.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/stringobject_factory.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

template <typename T>
struct is_callable
{
    template <typename U>
    static auto test(U* p) -> decltype(&U::operator(), std::true_type());

    template <typename U>
    static auto test(...) -> std::false_type;

    static const bool value = std::is_function<T>::value || std::is_member_function_pointer<T>::value || decltype(test<T>(nullptr))::value;
};

template <typename OnPacketReceivedCallbackType>
class EthernetItf
{
static_assert(is_callable<OnPacketReceivedCallbackType>::value, "OnPacketReceivedCallbackType must be callable");

public:
    virtual ~EthernetItf() = default;
    virtual ListPtr<StringPtr> getEthernetDevicesNamesList() = 0;
    virtual ListPtr<StringPtr> getEthernetDevicesDescriptionsList() = 0;
    virtual void sendPacket(const std::vector<uint8_t>& data) = 0;
    virtual void startCapture(OnPacketReceivedCallbackType packetReceivedCb) = 0;
    virtual void stopCapture() = 0;
    virtual bool isDeviceCapturing() const = 0;
    virtual bool setDevice(const StringPtr& deviceName) = 0;
};

END_NAMESPACE_ASAM_CMP_COMMON
