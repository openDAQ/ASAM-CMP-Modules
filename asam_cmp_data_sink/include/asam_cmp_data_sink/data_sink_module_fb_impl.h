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
#include <PcapLiveDeviceList.h>
#include <asam_cmp/decoder.h>
#include <opendaq/function_block_impl.h>

#include <asam_cmp_data_sink/common.h>

BEGIN_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE

class DataSinkModuleFbImpl final : public FunctionBlock
{
public:
    explicit DataSinkModuleFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~DataSinkModuleFbImpl() override;

    static FunctionBlockTypePtr CreateType();

private:
    void initProperties();
    void addNetworkAdaptersProperty();
    void createFbs();
    void startCapture();
    void stopCapture();
    void onPacketArrives(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie);
    std::vector<std::shared_ptr<ASAM::CMP::Packet>> decode(pcpp::RawPacket* packet);

    static void addDeviceDescription(ListPtr<StringPtr>& devicesNames, const StringPtr& name);

private:
    inline static const pcpp::MacAddress broadcastMac{"FF:FF:FF:FF:FF:FF"};
    static constexpr uint16_t asamCmpEtherType = 0x99FE;

private:
    pcpp::PcapLiveDeviceList& pcapDeviceList{pcpp::PcapLiveDeviceList::getInstance()};
    pcpp::PcapLiveDevice* pcapLiveDevice{nullptr};
    ASAM::CMP::Decoder decoder;
};

END_NAMESPACE_ASAM_CMP_DATA_SINK_MODULE
