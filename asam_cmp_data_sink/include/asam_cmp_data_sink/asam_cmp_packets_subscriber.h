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
#include <coretypes/baseobject.h>
#include <memory>

#include <asam_cmp_data_sink/common.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

DECLARE_OPENDAQ_INTERFACE(IAsamCmpPacketsSubscriber, IBaseObject)
{
    virtual void receive(const std::shared_ptr<ASAM::CMP::Packet>& packet) = 0;
    virtual void receive(const std::vector<std::shared_ptr<ASAM::CMP::Packet>>& packets) = 0;
};

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE