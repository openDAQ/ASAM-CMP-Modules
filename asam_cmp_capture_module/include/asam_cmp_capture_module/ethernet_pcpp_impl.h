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
#include <asam_cmp_capture_module/ethernet_itf.h>
#include <PcapLiveDeviceList.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

class EthernetPcppImpl : public EthernetItf<std::function<void(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)>>
{
public:
    ListPtr<StringPtr> getEthernatDevicesNamesList() override;
    ListPtr<StringPtr> getEthernatDevicesDescriptionsList() override;
    void sendPacket(const StringPtr& deviceName, const std::vector<uint8_t>& data) override;
    void startCapture(const StringPtr& deviceName,
                      std::function<void(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)> onPacketReceivedCb) override;
    void stopCapture(const StringPtr& deviceName) override;

private:
    pcpp::PcapLiveDevice* getPcapLiveDevice(StringPtr deviceName);

private:
    pcpp::PcapLiveDeviceList& pcapDeviceList{pcpp::PcapLiveDeviceList::getInstance()};
};

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
