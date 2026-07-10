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
    RandomBoolGenerator rng(PrepRNG(bitness, case_id));
    DecisionTree tree(bitness);
    std::vector<bool> path_used_bits(bitness, false);
    tree.BuildSubtree(
        case_id,
        path_used_bits,
        /*path_used_count=*/0,
        rng.Generate(),
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
    size_t reps,
    uint64_t seed,
    float* out)
{
    assert(case_ids != nullptr);
    assert(out != nullptr);

    const size_t sample_size = 2 * bitness + 1;

    for (size_t case_index = 0; case_index < cases; ++case_index) {
        const size_t case_id = case_ids[case_index];
        assert(case_id < bb_tree_cases_number(bitness));
        const DecisionTree& tree = GetDecisionTree(bitness, case_id);
        tree.FillValueTensor(
            reps,
            CaseInputSeed(seed, bitness, case_id),
            out + case_index * reps * sample_size);
    }
}

void bb_tree_restrictions_tensor(
    uint16_t bitness,
    const size_t* case_ids,
    size_t cases,
    size_t reps,
    uint64_t seed,
    float* out)
{
    assert(case_ids != nullptr);
    assert(out != nullptr);

    const size_t restrictions = 2 * bitness;
    const size_t sample_size = 2 * bitness - 1;
    for (size_t case_index = 0; case_index < cases; ++case_index) {
        const size_t case_id = case_ids[case_index];
        assert(case_id < bb_tree_cases_number(bitness));
        const DecisionTree& tree = GetDecisionTree(bitness, case_id);
        tree.FillRestrictionsTensor(
            reps,
            CaseInputSeed(seed, bitness, case_id),
            out + case_index * restrictions * reps * sample_size);
    }
}
