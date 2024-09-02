#pragma once
#include <asam_cmp/packet.h>
#include <coretypes/baseobject.h>
#include <memory>

#include <asam_cmp_data_sink/common.h>
#include <asam_cmp_data_sink/data_handler.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

class CallsMultiMap final
{
public:
    auto insert(uint16_t deviceId, uint32_t interfaceId, uint8_t streamId, IDataHandler* handler)
    {
        std::scoped_lock lock(callMapMutex);

        return callsMap.insert({{deviceId, interfaceId, streamId}, handler});
    }

    void erase(uint16_t deviceId, uint32_t interfaceId, uint8_t streamId, IDataHandler* handler)
    {
        std::scoped_lock lock(callMapMutex);

        auto range = callsMap.equal_range({deviceId, interfaceId, streamId});
        if (range.first == callsMap.end() && range.second == callsMap.end())
            return;

        auto it = std::find_if(range.first, range.second, [handler](const auto& val) { return val.second == handler; });

        if (it != callsMap.end())
            callsMap.erase(it);
    }

    void processPacket(const std::shared_ptr<ASAM::CMP::Packet>& packet)
    {
        std::scoped_lock lock(callMapMutex);

        auto range = callsMap.equal_range({packet->getDeviceId(), packet->getInterfaceId(), packet->getStreamId()});
        for (auto& it = range.first; it != range.second; ++it)
        {
            it->second->processData(packet);
        }
    }

    void processPackets(const std::vector<std::shared_ptr<ASAM::CMP::Packet>>& packets)
    {
        std::scoped_lock lock(callMapMutex);

        auto range =
            callsMap.equal_range({packets.front()->getDeviceId(), packets.front()->getInterfaceId(), packets.front()->getStreamId()});
        for (auto& it = range.first; it != range.second; ++it)
        {
            it->second->processData(packets);
        }
    }

    size_t size() const
    {
        std::scoped_lock lock(callMapMutex);
        return callsMap.size();
    }

private:
    struct Endpoint
    {
        uint16_t deviceId{0};
        uint32_t interfaceId{0};
        uint8_t streamId{0};

        bool operator==(const Endpoint& rhs) const
        {
            return (deviceId == rhs.deviceId) && (streamId == rhs.streamId) && (interfaceId == rhs.interfaceId);
        }
    };

    struct EndpointHash
    {
        uint64_t operator()(const Endpoint& rhs) const
        {
            return (rhs.deviceId | (static_cast<uint64_t>(rhs.interfaceId) << 16) | (static_cast<uint64_t>(rhs.streamId) << 48));
        }
    };

private:
    mutable std::mutex callMapMutex;
    std::unordered_multimap<Endpoint, IDataHandler*, EndpointHash> callsMap;
};

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
