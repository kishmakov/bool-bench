#pragma once

#include <functional>
#include <stddef.h>
#include <stdint.h>
#include <string_view>

inline constexpr uint16_t kMinTableBitness = 4;
inline constexpr uint16_t kMaxTableBitness = 256; // technical limitation for a while
inline constexpr uint16_t kSolvableTableBitness = 16;

using TableValueFunction = std::function<bool(std::string_view)>;

TableValueFunction MakeTableValueFunction(uint16_t bitness, size_t case_id);
