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
#include <asam_cmp/packet.h>
#include <asam_cmp/status.h>
#include <memory>
#include <mutex>
#include <coretypes/baseobject.h>

#include <asam_cmp_data_sink/common.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

class StatusMt final
{
public:
    StatusMt(const ASAM::CMP::Status& st, std::mutex& mt)
        : mtRef(mt)
        , statusRef(st)
    {
    }

    ASAM::CMP::Status getStatus() const
    {
        std::scoped_lock lock(mtRef);
        return statusRef;
    }

    ASAM::CMP::DeviceStatus getDeviceStatus(size_t index) const
    {
        std::scoped_lock lock(mtRef);
        return index < statusRef.getDeviceStatusCount() ? statusRef.getDeviceStatus(index) : ASAM::CMP::DeviceStatus{};
    }

private:
    std::mutex& mtRef;
    const ASAM::CMP::Status& statusRef;
};

DECLARE_OPENDAQ_INTERFACE(IStatusHandler, IBaseObject)
{
    virtual void processStatusPacket(const std::shared_ptr<ASAM::CMP::Packet>& packet) = 0;
    virtual StatusMt getStatusMt() const = 0;
};

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
