#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string_view>

inline constexpr size_t kTreeCasesNumber = size_t{1} << 32;
inline constexpr uint16_t kMinTreeBitness = 10;
inline constexpr uint16_t kMaxTreeBitness = 256;

bool EvaluateTreeCase(uint16_t bitness, size_t case_id, std::string_view input);
