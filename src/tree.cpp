#include "bool_bench.h"
#include "decision_tree.h"
#include "tree.h"
#include "utils.h"

#include <cassert>
#include <cstring>
#include <map>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using CaseKey = std::pair<uint16_t, size_t>;

std::map<CaseKey, DecisionTree> g_decision_trees;

DecisionTree BuildDecisionTree(uint16_t bitness, size_t case_id) {
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
    auto it = g_decision_trees.find(key);
    if (it == g_decision_trees.end()) {
        it = g_decision_trees.emplace(key, BuildDecisionTree(bitness, case_id)).first;
    }
    return it->second;
}

bool EvaluateTreeCase(
    uint16_t bitness,
    size_t case_id,
    std::string_view input)
{
    assert(input.size() == bitness);
    return GetDecisionTree(bitness, case_id).Evaluate(input);
}

}  // namespace

uint16_t bb_min_tree_bitness() {
    return kMinTreeBitness;
}

size_t bb_tree_cases_number(uint16_t bitness) {
    assert(bitness >= kMinTreeBitness && bitness <= kMaxTreeBitness);
    return kTreeCasesNumber;
}

size_t bb_tree_nodes(uint16_t bitness, size_t case_id) {
    assert(case_id < bb_tree_cases_number(bitness));

    const DecisionTree& tree = GetDecisionTree(bitness, case_id);
    return tree.nodes.size() - tree.num_leafs;
}

size_t bb_tree_depth(uint16_t bitness, size_t case_id) {
    assert(case_id < bb_tree_cases_number(bitness));

    const DecisionTree& tree = GetDecisionTree(bitness, case_id);
    return tree.depth;
}

const char* bb_tree_value(uint16_t bitness, size_t case_id, const char* input) {
    assert(case_id < bb_tree_cases_number(bitness));
    assert(input != nullptr);
    assert(std::strlen(input) == bitness);

    thread_local std::string value;
    thread_local FlippingSampler sampler;

    value.assign(2 * bitness + 1, '0');
    sampler.Reset(bitness, {input, bitness});

    const auto evaluate = [bitness, case_id](std::string_view point) {
        return EvaluateTreeCase(bitness, case_id, point);
    };
    sampler.Fill(
        value,
        /*sample_offset=*/0,
        bitness,
        evaluate);

    return value.c_str();
}

void bb_tree_value_tensor(
    uint16_t bitness,
    const size_t* case_ids,
    size_t cases,
    const char* packed_inputs,
    size_t reps,
    float* out)
{
    assert(case_ids != nullptr);
    assert(packed_inputs != nullptr);
    assert(out != nullptr);

    const size_t sample_size = 2 * bitness + 1;
    thread_local FlippingSampler sampler;

    for (size_t case_index = 0; case_index < cases; ++case_index) {
        const size_t case_id = case_ids[case_index];
        assert(case_id < bb_tree_cases_number(bitness));

        const auto evaluate = [bitness, case_id](std::string_view point) {
            return EvaluateTreeCase(bitness, case_id, point);
        };

        for (size_t rep = 0; rep < reps; ++rep) {
            const size_t input_offset = (case_index * reps + rep) * bitness;
            const size_t output_offset = (case_index * reps + rep) * sample_size;
            sampler.Reset(bitness, {packed_inputs + input_offset, bitness});
            sampler.Fill(
                out,
                output_offset,
                bitness,
                evaluate);
        }
    }
}

const char* bb_tree_restrictions(uint16_t bitness, size_t case_id, const char* input) {
    assert(case_id < bb_tree_cases_number(bitness));

    const auto evaluate = [bitness, case_id](std::string_view point) {
        return EvaluateTreeCase(bitness, case_id, point);
    };
    return FillRestrictions(bitness, input, evaluate);
}
