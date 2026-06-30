#pragma once

#include "decision_tree.h"

#include <stddef.h>
#include <stdint.h>

#include <vector>

inline constexpr uint16_t kMinMediumBitness = 7;
inline constexpr uint16_t kMaxMediumBitness = 16;

bool IsMediumBitness(uint16_t bitness);
size_t MediumBitnessCasesNumber(uint16_t bitness);

std::vector<bool> MediumBitnessTruthTable(uint16_t bitness, size_t case_id);
DecisionTree MediumBitnessDecisionTree(uint16_t bitness, size_t case_id);
size_t MediumBitnessCaseNodes(uint16_t bitness, size_t case_id);
size_t MediumBitnessCaseDepth(uint16_t bitness, size_t case_id);
