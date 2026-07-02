#include "aig.h"
#include "bool_bench.h"
#include "decision_tree.h"
#include "medium_bitness.h"
#include "small_bitness.h"
#include "table.h"
#include "utils.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

const size_t kCasesNumber = 1ull << 32; // some technical limitation

namespace {

using CaseKey = std::pair<uint16_t, size_t>;

std::map<CaseKey, std::vector<bool>> g_medium_truth_tables;
std::mutex g_medium_truth_tables_mutex;

std::map<CaseKey, DecisionTree> g_decision_trees;
std::mutex g_decision_trees_mutex;

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
    if (IsMediumBitness(bitness)) {
        return BuildSizeOptimalDecisionTree(
            bitness,
            GetMediumTruthTable(bitness, case_id));
    }

    std::mt19937 rng = PrepRNG(bitness, case_id);
    DecisionTree tree(bitness);
    std::vector<bool> path_used_bits(bitness, false);
    tree.BuildSubtree(
        case_id,
        path_used_bits,
        /*path_used_count=*/0,
        RandomBool(rng),
        rng);
    tree.Finalize();
    return tree;
}

const DecisionTree& GetDecisionTree(uint16_t bitness, size_t case_id) {
    assert(case_id < bb_tree_cases_number(bitness));

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

bool EvaluateGeneratedCase(
    uint16_t bitness,
    size_t case_id,
    std::string_view input)
{
    assert(input.size() == bitness);
    return GetDecisionTree(bitness, case_id).Evaluate(input);
}

}  // namespace

// API

size_t bb_tree_cases_number(uint16_t bitness) {
    if (IsSmallBitness(bitness)) {
        return SmallBitnessCasesNumber(bitness);
    }
    if (IsMediumBitness(bitness)) {
        return MediumBitnessCasesNumber(bitness);
    }
    return kCasesNumber;
}

size_t bb_gen_nodes(uint16_t bitness, size_t case_id) {
    assert(case_id < bb_tree_cases_number(bitness));

    const DecisionTree& tree = GetDecisionTree(bitness, case_id);
    return tree.nodes.size() - tree.num_leafs;
}

size_t bb_gen_depth(uint16_t bitness, size_t case_id) {
    assert(case_id < bb_tree_cases_number(bitness));

    const DecisionTree& tree = GetDecisionTree(bitness, case_id);
    return tree.depth;
}

const char* bb_gen_value(uint16_t bitness, size_t case_id, const char* input) {
    assert(case_id < bb_tree_cases_number(bitness));
    assert(input != nullptr);
    assert(std::strlen(input) == bitness);

    thread_local std::string value;
    thread_local FlippingSampler sampler;

    value.assign(2 * bitness + 1, '0');
    sampler.Reset(bitness, {input, bitness});

    const auto evaluate = [bitness, case_id](std::string_view point) {
        return EvaluateGeneratedCase(bitness, case_id, point);
    };
    sampler.Fill(
        value,
        /*sample_offset=*/0,
        bitness,
        evaluate);

    return value.c_str();
}

const char* bb_case_restrictions_impl(
    uint16_t bitness,
    size_t case_id,
    size_t rep,
    bool random_sparse);

const char* bb_case_restrictions(uint16_t bitness, size_t case_id, size_t rep) {
    return bb_case_restrictions_impl(bitness, case_id, rep, false);
}

const char* bb_case_restrictions_rnd(uint16_t bitness, size_t case_id, size_t rep) {
    return bb_case_restrictions_impl(bitness, case_id, rep, true);
}

const char* bb_case_restrictions_impl(
    uint16_t bitness,
    size_t case_id,
    size_t rep,
    bool random_sparse)
{
    assert(bitness > 0);
    if (random_sparse) {
        assert(bitness >= kMinTableBitness && bitness <= kMaxTableBitness);
        assert(case_id < bb_table_cases_number(bitness));
    } else {
        assert(case_id < bb_tree_cases_number(bitness));
    }

    thread_local std::string value;
    thread_local std::string sample_input;

    std::mt19937 rng = PrepRNG(bitness, case_id);

    const size_t free_bits = bitness - 1;
    const size_t sample_size = 2 * free_bits + 1;
    value.assign(bitness * 2 * sample_size, '0');
    sample_input.assign(bitness, '0');

    const std::function<bool(std::string_view)> evaluate =
        random_sparse
            ? MakeTableValueFunction(bitness, case_id)
            : std::function<bool(std::string_view)>(
                [bitness, case_id](std::string_view point) {
                    return EvaluateGeneratedCase(bitness, case_id, point);
                });

    size_t offset = 0;
    for (size_t fixed_bit_id = 0; fixed_bit_id < bitness; ++fixed_bit_id) {
        for (size_t fixed_bit_value = 0; fixed_bit_value <= 1; ++fixed_bit_value) {
            // Pin the fixed bit
            sample_input[fixed_bit_id] = static_cast<char>('0' + fixed_bit_value);

            // Sample random values for the free bits
            for (size_t coord = 0; coord < free_bits; ++coord) {
                sample_input[FullBitId(coord, fixed_bit_id)] = RandomBool(rng) ? '1' : '0';
            }

            FlippingSampler sampler(bitness, sample_input);
            sampler.Fill(
                value,
                offset,
                fixed_bit_id,
                evaluate);
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
