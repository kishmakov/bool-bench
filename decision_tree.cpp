#include "decision_tree.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace {

struct SelectedBits {
    uint16_t mask = 0;
    uint16_t values = 0;

    bool operator==(const SelectedBits& other) const {
        return mask == other.mask && values == other.values;
    }
};

struct SelectedBitsHash {
    size_t operator()(SelectedBits bits) const {
        return static_cast<size_t>(bits.mask) | (static_cast<size_t>(bits.values) << 16);
    }
};

struct StateStats {
    uint8_t seen = 0;
};

struct StatePlan {
    bool is_leaf = true;
    bool value = false;
    uint16_t bit_id = 0;
    size_t depth = 0;
    size_t size = 0;
};

enum class TruthTableObjective {
    Depth,
    Size,
};

SelectedBits WithBit(SelectedBits bits, uint16_t bit_id, bool value) {
    const uint16_t bit = static_cast<uint16_t>(1u << bit_id);
    bits.mask |= bit;
    if (value) {
        bits.values |= bit;
    } else {
        bits.values &= static_cast<uint16_t>(~bit);
    }
    return bits;
}

bool IsBetterPlan(const StatePlan& candidate, const StatePlan& best, TruthTableObjective objective) {
    if (objective == TruthTableObjective::Depth) {
        return std::tie(candidate.depth, candidate.size, candidate.bit_id) <
            std::tie(best.depth, best.size, best.bit_id);
    }
    return std::tie(candidate.size, candidate.depth, candidate.bit_id) <
        std::tie(best.size, best.depth, best.bit_id);
}

class TruthTableTreeBuilder {
public:
    TruthTableTreeBuilder(uint16_t bitness, uint64_t truth_table, TruthTableObjective objective)
        : bitness_(bitness)
        , truth_table_(truth_table)
        , objective_(objective)
        , tree_(bitness)
    {
        assert(bitness_ <= 6);
    }

    DecisionTree Build() {
        tree_.nodes.push_back(false);
        BuildNodes(0, {});
        tree_.Finalize();
        return std::move(tree_);
    }

private:
    void BuildNodes(size_t node_id, SelectedBits bits) {
        const StatePlan plan = ChoosePlan(bits);

        if (plan.is_leaf) {
            tree_.nodes[node_id] = plan.value;
            ++tree_.num_leafs;
            return;
        }

        const size_t child0 = tree_.nodes.size();
        const size_t child1 = tree_.nodes.size() + 1;
        tree_.nodes[node_id] = Div{plan.bit_id, child0, child1};
        tree_.used_bits[plan.bit_id] = true;
        tree_.nodes.push_back(false);
        tree_.nodes.push_back(false);

        BuildNodes(child0, WithBit(bits, plan.bit_id, false));
        BuildNodes(child1, WithBit(bits, plan.bit_id, true));
    }

    StateStats Analyze(SelectedBits bits) {
        auto cached = stats_memo_.find(bits);
        if (cached != stats_memo_.end()) {
            return cached->second;
        }

        StateStats stats;
        const uint16_t all_bits = static_cast<uint16_t>((1u << bitness_) - 1);
        const uint16_t free_mask = static_cast<uint16_t>(all_bits ^ bits.mask);
        uint16_t sub = free_mask;
        do {
            const uint16_t input_id = static_cast<uint16_t>(sub | bits.values);
            stats.seen |= static_cast<uint8_t>(1u << TableValue(input_id));
            sub = static_cast<uint16_t>((sub - 1) & free_mask);
        } while (sub != free_mask);

        stats_memo_[bits] = stats;
        return stats;
    }

    StatePlan ChoosePlan(SelectedBits bits) {
        auto cached = plan_memo_.find(bits);
        if (cached != plan_memo_.end()) {
            return cached->second;
        }

        const StateStats stats = Analyze(bits);
        if (stats.seen != 3) {
            const StatePlan plan{true, stats.seen == 2, 0, 0, 0};
            plan_memo_[bits] = plan;
            return plan;
        }

        StatePlan best;
        best.is_leaf = false;
        best.depth = std::numeric_limits<size_t>::max();
        best.size = std::numeric_limits<size_t>::max();

        for (uint16_t bit_id = 0; bit_id < bitness_; ++bit_id) {
            if ((bits.mask & (1u << bit_id)) != 0) {
                continue;
            }

            const StatePlan left = ChoosePlan(WithBit(bits, bit_id, false));
            const StatePlan right = ChoosePlan(WithBit(bits, bit_id, true));
            const StatePlan candidate{
                false,
                false,
                bit_id,
                1 + std::max(left.depth, right.depth),
                1 + left.size + right.size,
            };
            if (IsBetterPlan(candidate, best, objective_)) {
                best = candidate;
            }
        }

        plan_memo_[bits] = best;
        return best;
    }

    bool TableValue(uint16_t input_id) const {
        return ((truth_table_ >> input_id) & 1ull) != 0;
    }

    uint16_t bitness_;
    uint64_t truth_table_;
    TruthTableObjective objective_;
    DecisionTree tree_;
    std::unordered_map<SelectedBits, StateStats, SelectedBitsHash> stats_memo_;
    std::unordered_map<SelectedBits, StatePlan, SelectedBitsHash> plan_memo_;
};

size_t RandomUnusedBit(const std::vector<bool>& path_used_bits, size_t free_bits, std::mt19937& rng) {
    assert(free_bits > 0);

    const size_t selected = std::uniform_int_distribution<size_t>(0, free_bits - 1)(rng);
    size_t seen = 0;
    for (size_t bit = 0; bit < path_used_bits.size(); ++bit) {
        if (path_used_bits[bit]) continue;
        if (seen == selected) return bit;
        ++seen;
    }
    assert(false);
    return 0;
}

bool RandomBool(std::mt19937& rng) {
    return std::uniform_int_distribution<int>(0, 1)(rng) != 0;
}

std::pair<size_t, size_t> SplitBudget(size_t n, std::mt19937& rng) {
    assert(n >= 1);
    const size_t remaining = n - 1;
    if (remaining == 0) return {0, 0};
    const size_t left = std::uniform_int_distribution<size_t>(0, remaining)(rng);
    return {left, remaining - left};
}

size_t MaxInternalNodes(size_t free_bits) {
    if (free_bits >= std::numeric_limits<size_t>::digits) {
        return std::numeric_limits<size_t>::max();
    }
    return (size_t{1} << free_bits) - 1;
}

size_t ComputeDepth(const std::vector<Node>& nodes, size_t node_id) {
    const Node& node = nodes[node_id];
    const Div* division = std::get_if<Div>(&node);
    if (division == nullptr) {
        return 0;
    }
    return 1 + std::max(
        ComputeDepth(nodes, division->child0),
        ComputeDepth(nodes, division->child1));
}

}  // namespace

DecisionTree::DecisionTree(uint16_t bitness)
    : used_bits(bitness, false)
    , bitness_(bitness)
{
}

uint16_t DecisionTree::Bitness() const {
    return bitness_;
}

size_t DecisionTree::AddLeaf(bool value) {
    const size_t node_id = nodes.size();
    nodes.push_back(value);
    ++num_leafs;
    return node_id;
}

size_t DecisionTree::BuildSubtree(
    size_t budget,
    std::vector<bool>& path_used_bits,
    size_t path_used_count,
    bool required_value,
    std::mt19937& rng)
{
    assert(path_used_bits.size() == bitness_);
    assert(path_used_count <= bitness_);

    const size_t free_bits = bitness_ - path_used_count;
    if (budget == 0 || free_bits == 0) {
        return AddLeaf(required_value);
    }

    const size_t node_id = nodes.size();
    nodes.push_back(false);

    const size_t bit_id = RandomUnusedBit(path_used_bits, free_bits, rng);
    used_bits[bit_id] = true;
    path_used_bits[bit_id] = true;

    auto [left_budget, right_budget] = SplitBudget(budget, rng);

    const size_t max_child_nodes = MaxInternalNodes(free_bits - 1);
    left_budget = std::min(left_budget, max_child_nodes);
    right_budget = std::min(right_budget, max_child_nodes);

    const bool child0_required_value = RandomBool(rng);
    const bool child1_required_value = !child0_required_value;

    const size_t child0 = BuildSubtree(
        left_budget,
        path_used_bits,
        path_used_count + 1,
        child0_required_value,
        rng);
    const size_t child1 = BuildSubtree(
        right_budget,
        path_used_bits,
        path_used_count + 1,
        child1_required_value,
        rng);
    path_used_bits[bit_id] = false;
    nodes[node_id] = Div{bit_id, child0, child1};
    return node_id;
}

void DecisionTree::Finalize() {
    assert(!nodes.empty());
    depth = ComputeDepth(nodes, 0);
}

bool DecisionTree::Evaluate(std::string_view input) const {
    assert(input.size() == bitness_);

    size_t node_id = 0;
    while (true) {
        const Node& node = nodes[node_id];
        const Div* division = std::get_if<Div>(&node);
        if (division == nullptr) {
            return std::get<bool>(node);
        }
        assert(division->bitId < input.size());
        node_id = input[division->bitId] == '1'
            ? division->child1
            : division->child0;
    }
}

DecisionTree BuildDepthOptimalDecisionTree(uint16_t bitness, uint64_t truth_table) {
    return TruthTableTreeBuilder(
        bitness,
        truth_table,
        TruthTableObjective::Depth).Build();
}

DecisionTree BuildSizeOptimalDecisionTree(uint16_t bitness, uint64_t truth_table) {
    return TruthTableTreeBuilder(
        bitness,
        truth_table,
        TruthTableObjective::Size).Build();
}
