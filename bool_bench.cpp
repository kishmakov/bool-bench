#include "aig.h"
#include "bool_bench.h"
#include "decision_tree.h"
#include "medium_bitness.h"
#include "small_bitness.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

const size_t kCasesNumber = 1ull << 32; // some technical limitation

const size_t kMaxEffectiveSize = (1u << 16); // exclusive upper bound

namespace {

bool RandomBool(std::mt19937& rng) {
    return std::uniform_int_distribution<int>(0, 1)(rng) != 0;
}

DecisionTree RandomTree(uint16_t bitness, std::mt19937& rng, size_t target_size) {
    DecisionTree tree(bitness);

    if (target_size == 0) { // Pure leaf tree
        tree.AddLeaf(RandomBool(rng));
        tree.Finalize();
        return tree;
    }

    std::vector<bool> path_used_bits(bitness, false);
    tree.BuildSubtree(
        target_size,
        path_used_bits,
        /*path_used_count=*/0,
        RandomBool(rng),
        rng);
    tree.Finalize();
    return tree;
}

inline std::mt19937 PrepRNG(uint16_t bitness, size_t case_id) {
    std::seed_seq seed{
        static_cast<uint32_t>(bitness),
        static_cast<uint32_t>(case_id),
    };
    return std::mt19937(seed);
}

inline size_t PrepSize(uint16_t bitness, std::mt19937& rng) {
    const size_t max_size = bitness >= 16
        ? kMaxEffectiveSize - 1
        : (1ull << bitness) - 1;
    return std::uniform_int_distribution<size_t>(0, max_size)(rng);
}

// Computes position of bit in masked sequence
inline size_t FullBitId(size_t bit_id, size_t fixed_id) {
    return bit_id < fixed_id ? bit_id : bit_id + 1;
}

void WriteCompactFlipSample(
    std::string& value,
    size_t sample_offset,
    std::string& input,
    const DecisionTree& tree,
    size_t fixed_bit_id)
{
    const size_t bitness = input.size();
    const size_t free_bits = fixed_bit_id < bitness ? bitness - 1 : bitness;

    for (size_t coord = 0; coord < free_bits; ++coord) {
        value[sample_offset + coord] = input[FullBitId(coord, fixed_bit_id)];
    }
    value[sample_offset + free_bits] = tree.Evaluate({input.data(), bitness}) ? '1' : '0';
    for (size_t coord = 0; coord < free_bits; ++coord) {
        char& bit = input[FullBitId(coord, fixed_bit_id)];
        bit = bit == '1' ? '0' : '1';
        value[sample_offset + free_bits + 1 + coord] = tree.Evaluate({input.data(), bitness}) ? '1' : '0';
        bit = bit == '1' ? '0' : '1';
    }
}

}  // namespace

const DecisionTree& GetRandomTree(uint16_t bitness, size_t case_id) {
    using CaseKey = std::pair<uint16_t, size_t>;

    static std::map<CaseKey, DecisionTree> generated_trees;
    static std::mutex generated_trees_mutex;

    std::lock_guard<std::mutex> lock(generated_trees_mutex);

    const CaseKey key{bitness, case_id};
    auto it = generated_trees.find(key);
    if (it == generated_trees.end()) {
        if (IsSmallBitness(bitness)) {
            it = generated_trees.emplace(
                key,
                BuildSizeOptimalDecisionTree(
                    bitness,
                    SmallBitnessTruthTable(bitness, case_id))).first;
        } else if (IsMediumBitness(bitness)) {
            it = generated_trees.emplace(
                key,
                MediumBitnessDecisionTree(bitness, case_id)).first;
        } else {
            std::mt19937 rng = PrepRNG(bitness, case_id);
            const size_t size = PrepSize(bitness, rng);
            it = generated_trees.emplace(key, RandomTree(bitness, rng, size)).first;
        }
    }
    return it->second;
}

// API

size_t bb_gen_cases_number(uint16_t bitness) {
    if (IsSmallBitness(bitness)) {
        return SmallBitnessCasesNumber(bitness);
    }
    if (IsMediumBitness(bitness)) {
        return MediumBitnessCasesNumber(bitness);
    }
    return kCasesNumber;
}

size_t bb_gen_nodes(uint16_t bitness, size_t case_id) {
    assert(case_id < bb_gen_cases_number(bitness));

    if (IsSmallBitness(bitness)) {
        return SmallBitnessCaseNodes(bitness, case_id);
    }

    const DecisionTree& tree = GetRandomTree(bitness, case_id);
    return tree.nodes.size() - tree.num_leafs;
}

size_t bb_gen_depth(uint16_t bitness, size_t case_id) {
    assert(case_id < bb_gen_cases_number(bitness));

    if (IsSmallBitness(bitness)) {
        return SmallBitnessCaseDepth(bitness, case_id);
    }

    const DecisionTree& tree = GetRandomTree(bitness, case_id);
    return tree.depth;
}

const char* bb_gen_value(uint16_t bitness, size_t case_id, const char* input) {
    assert(case_id < bb_gen_cases_number(bitness));
    assert(input != nullptr);
    assert(std::strlen(input) == bitness);

    thread_local std::string value;
    thread_local std::string point_input;

    value.assign(2 * bitness + 1, '0');
    point_input.assign(input, bitness);

    const DecisionTree& tree = GetRandomTree(bitness, case_id);
    WriteCompactFlipSample(value, /*sample_offset=*/0, point_input, tree, bitness);

    return value.c_str();
}

const char* bb_case_restrictions(uint16_t bitness, size_t case_id, size_t rep) {
    assert(case_id < bb_gen_cases_number(bitness));
    assert(bitness > 0);

    thread_local std::string value;
    thread_local std::string input;

    std::mt19937 rng = PrepRNG(bitness, case_id);

    const size_t free_bits = bitness - 1;
    const size_t sample_size = 2 * free_bits + 1;
    value.assign(bitness * 2 * sample_size, '0');
    input.assign(bitness, '0');

    const DecisionTree& tree = GetRandomTree(bitness, case_id);

    size_t offset = 0;
    for (size_t fixed_bit_id = 0; fixed_bit_id < bitness; ++fixed_bit_id) {
        for (size_t fixed_bit_value = 0; fixed_bit_value <= 1; ++fixed_bit_value) {
            // Pin the fixed bit
            input[fixed_bit_id] = static_cast<char>('0' + fixed_bit_value);

            // Sample random values for the free bits
            for (size_t coord = 0; coord < free_bits; ++coord) {
                input[FullBitId(coord, fixed_bit_id)] = RandomBool(rng) ? '1' : '0';
            }

            WriteCompactFlipSample(value, offset, input, tree, fixed_bit_id);
            offset += sample_size;
        }
    }

    return value.c_str();
}

const char* bb_circuit_sets() {
    return CircuitSets().c_str();
}

const char* bb_circuit_cases(const char* set_name) {
    return CircuitCases(set_name).c_str();
}

size_t bb_circuit_inputs(const char* set_name, const char* case_name) {
    return CircuitInputs(set_name, case_name);
}

size_t bb_circuit_outputs(const char* set_name, const char* case_name) {
    return CircuitOutputs(set_name, case_name);
}

const char* bb_circuit_value(const char* set_name, const char* case_name, const char* input_state) {
    return CircuitValue(set_name, case_name, input_state);
}
