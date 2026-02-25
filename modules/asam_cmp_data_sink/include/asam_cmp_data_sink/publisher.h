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
#include <algorithm>

#include <asam_cmp_data_sink/common.h>
#include <asam_cmp_data_sink/asam_cmp_packets_subscriber.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

template <typename Topic, typename Subscriber, class TopicHasher = std::hash<Topic>>
class Publisher final
{
public:
    auto subscribe(const Topic& topic, Subscriber* subscriber)
    {
        std::scoped_lock lock(subscribersMt);

        return subscribers.insert({topic, subscriber});
    }

    void unsubscribe(const Topic& topic, Subscriber* subscriber)
    {
        std::scoped_lock lock(subscribersMt);

        auto range = subscribers.equal_range(topic);
        if (range.first == subscribers.end() && range.second == subscribers.end())
            return;

        auto it = std::find_if(range.first, range.second, [subscriber](const auto& val) { return val.second == subscriber; });

        if (it != subscribers.end())
            subscribers.erase(it);
    }

    void publish(const Topic& topic, const std::shared_ptr<ASAM::CMP::Packet>& packet)
    {
        std::scoped_lock lock(subscribersMt);

        auto range = subscribers.equal_range(topic);
        for (auto& it = range.first; it != range.second; ++it)
        {
            it->second->receive(packet);
        }
    }

    void publish(const Topic& topic, const std::vector<std::shared_ptr<ASAM::CMP::Packet>>& packets)
    {
        std::scoped_lock lock(subscribersMt);

        auto range = subscribers.equal_range(topic);
        for (auto& it = range.first; it != range.second; ++it)
        {
            it->second->receive(packets);
        }
    }

    size_t size() const
    {
        std::scoped_lock lock(subscribersMt);
        return subscribers.size();
    }

private:
    mutable std::mutex subscribersMt;
    std::unordered_multimap<Topic, Subscriber*, TopicHasher> subscribers;
};

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
