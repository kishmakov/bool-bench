#include "small_bitness.h"

#include "decision_tree.h"

#include <cassert>
#include <cstdint>
#include <limits>
#include <random>

namespace {

uint64_t TruthTableMask(uint16_t bitness) {
    assert(IsSmallBitness(bitness));
    switch (bitness) {
        case 4: return 0xffffull;
        case 5: return 0xffffffffull;
        case 6: return 0xffffffffffffffffull;
        default:  assert(false); return 0;
    }
}

}  // namespace

bool IsSmallBitness(uint16_t bitness) {
    return kMinSmallBitness <= bitness && bitness <= kMaxSmallBitness;
}

size_t SmallBitnessCasesNumber(uint16_t bitness) {
    assert(IsSmallBitness(bitness));
    switch (bitness) {
        case 4: return 0x10000ull;
        case 5: return 0xffffffffull;
        case 6: return 0xffffffffull;
        default: assert(false); return 0;
    }
}

uint64_t SmallBitnessTruthTable(uint16_t bitness, size_t case_id) {
    assert(IsSmallBitness(bitness));
    assert(case_id < SmallBitnessCasesNumber(bitness));

    std::seed_seq seed{
        static_cast<uint32_t>(bitness),
        static_cast<uint32_t>(case_id),
    };
    std::mt19937_64 rng(seed);
    return rng() & TruthTableMask(bitness);
}

size_t SmallBitnessCaseNodes(uint16_t bitness, size_t case_id) {
    const DecisionTree tree = BuildSizeOptimalDecisionTree(
        bitness,
        SmallBitnessTruthTable(bitness, case_id));
    return tree.nodes.size() - tree.num_leafs;
}

size_t SmallBitnessCaseDepth(uint16_t bitness, size_t case_id) {
    const DecisionTree tree = BuildDepthOptimalDecisionTree(
        bitness,
        SmallBitnessTruthTable(bitness, case_id));
    return tree.depth;
}
