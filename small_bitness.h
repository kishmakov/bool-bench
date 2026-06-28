#pragma once

#include <stddef.h>
#include <stdint.h>

inline constexpr uint16_t kMinSmallBitness = 4;
inline constexpr uint16_t kMaxSmallBitness = 6;

bool IsSmallBitness(uint16_t bitness);
size_t SmallBitnessCasesNumber(uint16_t bitness);

uint64_t SmallBitnessTruthTable(uint16_t bitness, size_t case_id);
size_t SmallBitnessCaseNodes(uint16_t bitness, size_t case_id);
size_t SmallBitnessCaseDepth(uint16_t bitness, size_t case_id);
