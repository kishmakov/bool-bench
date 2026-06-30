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
#include <tuple>
#include <utility>
#include <vector>

const size_t kCasesNumber = 1ull << 32; // some technical limitation

namespace {

using CaseKey = std::pair<uint16_t, size_t>;
using ValueKey = std::tuple<uint16_t, size_t, std::vector<bool>>;

std::map<CaseKey, std::vector<bool>> g_medium_truth_tables;
std::mutex g_medium_truth_tables_mutex;

std::map<CaseKey, DecisionTree> g_decision_trees;
std::mutex g_decision_trees_mutex;

std::map<ValueKey, bool> g_sparse_values;
std::mutex g_sparse_values_mutex;

bool RandomBool(std::mt19937& rng) {
    return std::uniform_int_distribution<int>(0, 1)(rng) != 0;
}

inline std::mt19937 PrepRNG(uint16_t bitness, size_t case_id) {
    std::seed_seq seed{
        static_cast<uint32_t>(bitness),
        static_cast<uint32_t>(case_id),
    };
    return std::mt19937(seed);
}

// Computes position of bit in masked sequence
inline size_t FullBitId(size_t bit_id, size_t fixed_id) {
    return bit_id < fixed_id ? bit_id : bit_id + 1;
}

std::vector<bool> InputBits(std::string_view input) {
    std::vector<bool> bits;
    bits.reserve(input.size());
    for (char bit : input) {
        assert(bit == '0' || bit == '1');
        bits.push_back(bit == '1');
    }
    return bits;
}

const std::vector<bool>& GetMediumTruthTable(uint16_t bitness, size_t case_id) {
    assert(IsMediumBitness(bitness));
    assert(case_id < MediumBitnessCasesNumber(bitness));

    std::lock_guard<std::mutex> lock(g_medium_truth_tables_mutex);

    const CaseKey key{bitness, case_id};
    auto it = g_medium_truth_tables.find(key);
    if (it == g_medium_truth_tables.end()) {
        it = g_medium_truth_tables.emplace(
            key,
            MediumBitnessTruthTable(bitness, case_id)).first;
    }
    return it->second;
}

DecisionTree BuildDecisionTree(uint16_t bitness, size_t case_id) {
    if (IsSmallBitness(bitness)) {
        return BuildSizeOptimalDecisionTree(
            bitness,
            SmallBitnessTruthTable(bitness, case_id));
    }
    assert(IsMediumBitness(bitness));
    return BuildSizeOptimalDecisionTree(
        bitness,
        GetMediumTruthTable(bitness, case_id));
}

const DecisionTree& GetDecisionTree(uint16_t bitness, size_t case_id) {
    assert(IsSmallBitness(bitness) || IsMediumBitness(bitness));
    assert(case_id < bb_gen_cases_number(bitness));

    const CaseKey key{bitness, case_id};
    {
        std::lock_guard<std::mutex> lock(g_decision_trees_mutex);
        auto it = g_decision_trees.find(key);
        if (it != g_decision_trees.end()) {
            return it->second;
        }
    }

    DecisionTree tree = BuildDecisionTree(bitness, case_id);

    std::lock_guard<std::mutex> lock(g_decision_trees_mutex);
    auto it = g_decision_trees.find(key);
    if (it == g_decision_trees.end()) {
        it = g_decision_trees.emplace(key, std::move(tree)).first;
    }
    return it->second;
}

bool DeterministicSparseValue(
    uint16_t bitness,
    size_t case_id,
    const std::vector<bool>& input)
{
    assert(input.size() == bitness);

    const uint64_t case_seed = static_cast<uint64_t>(case_id);
    std::vector<uint32_t> seed{
        static_cast<uint32_t>(bitness),
        static_cast<uint32_t>(case_seed),
        static_cast<uint32_t>(case_seed >> 32),
    };

    uint32_t chunk = 0;
    uint32_t chunk_bits = 0;
    for (bool bit : input) {
        if (bit) {
            chunk |= uint32_t{1} << chunk_bits;
        }
        ++chunk_bits;
        if (chunk_bits == 32) {
            seed.push_back(chunk);
            chunk = 0;
            chunk_bits = 0;
        }
    }
    if (chunk_bits > 0) {
        seed.push_back(chunk);
    }

    std::seed_seq seed_seq(seed.begin(), seed.end());
    std::mt19937 rng(seed_seq);
    return RandomBool(rng);
}

bool GetSparseValue(uint16_t bitness, size_t case_id, std::string_view input) {
    assert(!IsSmallBitness(bitness));
    assert(!IsMediumBitness(bitness));
    assert(case_id < bb_gen_cases_number(bitness));
    assert(input.size() == bitness);

    const std::vector<bool> input_bits = InputBits(input);
    const ValueKey key{bitness, case_id, input_bits};
    std::lock_guard<std::mutex> lock(g_sparse_values_mutex);

    auto it = g_sparse_values.find(key);
    if (it == g_sparse_values.end()) {
        it = g_sparse_values.emplace(
            key,
            DeterministicSparseValue(bitness, case_id, input_bits)).first;
    }
    return it->second;
}

bool EvaluateGeneratedCase(
    uint16_t bitness,
    size_t case_id,
    std::string_view input)
{
    assert(input.size() == bitness);
    if (IsSmallBitness(bitness) || IsMediumBitness(bitness)) {
        return GetDecisionTree(bitness, case_id).Evaluate(input);
    }
    return GetSparseValue(bitness, case_id, input);
}

void WriteCompactFlipSample(
    std::string& value,
    size_t sample_offset,
    std::string& input,
    uint16_t bitness,
    size_t case_id,
    size_t fixed_bit_id)
{
    const size_t free_bits = fixed_bit_id < bitness ? bitness - 1 : bitness;

    for (size_t coord = 0; coord < free_bits; ++coord) {
        value[sample_offset + coord] = input[FullBitId(coord, fixed_bit_id)];
    }
    value[sample_offset + free_bits] =
        EvaluateGeneratedCase(bitness, case_id, {input.data(), bitness}) ? '1' : '0';
    for (size_t coord = 0; coord < free_bits; ++coord) {
        char& bit = input[FullBitId(coord, fixed_bit_id)];
        bit = bit == '1' ? '0' : '1';
        value[sample_offset + free_bits + 1 + coord] =
            EvaluateGeneratedCase(bitness, case_id, {input.data(), bitness}) ? '1' : '0';
        bit = bit == '1' ? '0' : '1';
    }
}

}  // namespace

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
    assert(IsSmallBitness(bitness) || IsMediumBitness(bitness));
    assert(case_id < bb_gen_cases_number(bitness));

    const DecisionTree& tree = GetDecisionTree(bitness, case_id);
    return tree.nodes.size() - tree.num_leafs;
}

size_t bb_gen_depth(uint16_t bitness, size_t case_id) {
    assert(IsSmallBitness(bitness) || IsMediumBitness(bitness));
    assert(case_id < bb_gen_cases_number(bitness));

    const DecisionTree& tree = GetDecisionTree(bitness, case_id);
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

    WriteCompactFlipSample(
        value,
        /*sample_offset=*/0,
        point_input,
        bitness,
        case_id,
        bitness);

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

    size_t offset = 0;
    for (size_t fixed_bit_id = 0; fixed_bit_id < bitness; ++fixed_bit_id) {
        for (size_t fixed_bit_value = 0; fixed_bit_value <= 1; ++fixed_bit_value) {
            // Pin the fixed bit
            input[fixed_bit_id] = static_cast<char>('0' + fixed_bit_value);

            // Sample random values for the free bits
            for (size_t coord = 0; coord < free_bits; ++coord) {
                input[FullBitId(coord, fixed_bit_id)] = RandomBool(rng) ? '1' : '0';
            }

            WriteCompactFlipSample(
                value,
                offset,
                input,
                bitness,
                case_id,
                fixed_bit_id);
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
