#pragma once

#include <asam_cmp_common_lib/common.h>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <array>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

namespace Units {
    uint8_t getIdBySymbol(const std::string& symbol) noexcept;
    std::string getSymbolById(uint8_t id) noexcept;
}

END_NAMESPACE_ASAM_CMP_COMMON
