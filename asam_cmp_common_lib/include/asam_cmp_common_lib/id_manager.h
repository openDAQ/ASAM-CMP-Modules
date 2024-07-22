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
#include <asam_cmp_common_lib/common.h>
#include <set>
#include <unordered_set>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

template <typename T>
class IdManager
{
public:
    T getFirstUnusedId()
    {
        return (unusedIds.empty() ? getMinUnusedId() : *(unusedIds.begin()));
    }

    bool isValidId(int64_t arg)
    {
        return (arg >= 0 && arg <= std::numeric_limits<T>::max() && usedIds.count(arg) == 0);
    }

    void addId(T id)
    {
        if (isValidId(id))
        {
            if (unusedIds.count(id))
                unusedIds.erase(id);

            usedIds.insert(id);
        }
    }

    void removeId(T id)
    {
        if (usedIds.count(id))
        {
            unusedIds.insert(id);
            usedIds.erase(id);
        }
    }

private:
    T getMinUnusedId() const
    {
        T result = 1;
        for (const auto& e : usedIds)
            result += (result == e);

        return result;
    }

    std::set<T> usedIds;
    std::unordered_set<T> unusedIds;
};

using AsamCmpInterfaceIdManager = IdManager<uint32_t>;
using AsamCmpStreamIdManager = IdManager<uint8_t>;
using AsamCmpInterfaceIdManagerPtr = AsamCmpInterfaceIdManager*;
using AsamCmpStreamIdManagerPtr = AsamCmpStreamIdManager*;

END_NAMESPACE_ASAM_CMP_COMMON
