#include "medium_bitness.h"

#include <cassert>
#include <cstdint>
#include <random>

namespace {

inline constexpr size_t kMediumCasesNumber = size_t{1} << 32;

}  // namespace

bool IsMediumBitness(uint16_t bitness) {
    return kMinMediumBitness <= bitness && bitness <= kMaxMediumBitness;
}

size_t MediumBitnessCasesNumber(uint16_t bitness) {
    assert(IsMediumBitness(bitness));
    return kMediumCasesNumber;
}

std::vector<bool> MediumBitnessTruthTable(uint16_t bitness, size_t case_id) {
    assert(IsMediumBitness(bitness));
    assert(case_id < MediumBitnessCasesNumber(bitness));

    std::vector<bool> truth_table(size_t{1} << bitness);
    std::seed_seq seed{
        static_cast<uint32_t>(bitness),
        static_cast<uint32_t>(case_id),
    };
    std::mt19937_64 rng(seed);

    size_t input_id = 0;
    while (input_id < truth_table.size()) {
        const uint64_t chunk = rng();
        for (size_t bit = 0; bit < 64 && input_id < truth_table.size(); ++bit) {
            truth_table[input_id] = ((chunk >> bit) & 1ull) != 0;
            ++input_id;
        }
    }
    return truth_table;
}

DecisionTree MediumBitnessDecisionTree(uint16_t bitness, size_t case_id) {
    return BuildSizeOptimalDecisionTree(
        bitness,
        MediumBitnessTruthTable(bitness, case_id));
}

size_t MediumBitnessCaseNodes(uint16_t bitness, size_t case_id) {
    const DecisionTree tree = MediumBitnessDecisionTree(bitness, case_id);
    return tree.nodes.size() - tree.num_leafs;
}

size_t MediumBitnessCaseDepth(uint16_t bitness, size_t case_id) {
    const DecisionTree tree = MediumBitnessDecisionTree(bitness, case_id);
    return tree.depth;
}
