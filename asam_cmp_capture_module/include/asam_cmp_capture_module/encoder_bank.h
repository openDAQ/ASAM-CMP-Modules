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
#include <asam_cmp/packet.h>
#include <asam_cmp/encoder.h>
#include <array>
#include <mutex>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

class EncoderBank;
using EncoderBankPtr = EncoderBank*;

class EncoderBank
{
public:
    void init(uint16_t deviceId);

    template <typename ForwardIterator>
    std::vector<std::vector<uint8_t>> encode(uint8_t encoderInd, ForwardIterator begin, ForwardIterator end, const ASAM::CMP::DataContext& dataContext);
    std::vector<std::vector<uint8_t>> encode(uint8_t encoderInd, const ASAM::CMP::Packet& packet, const ASAM::CMP::DataContext& dataContext);

private:
    static constexpr size_t encodersCount = 256;
    std::array<ASAM::CMP::Encoder, encodersCount> encoders;
    std::array<std::mutex, encodersCount> encoderSyncs;
};

template <typename ForwardIterator>
std::vector<std::vector<uint8_t>> EncoderBank::encode(uint8_t encoderInd,
                                         ForwardIterator begin,
                                         ForwardIterator end,
                                         const ASAM::CMP::DataContext& dataContext)
{
    std::scoped_lock lock(encoderSyncs[encoderInd]);
    return encoders[encoderInd].encode(begin, end, dataContext);
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
