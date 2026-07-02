#include "bool_bench.h"
#include "medium_bitness.h"
#include "small_bitness.h"
#include "table.h"
#include "utils.h"

#include <cassert>
#include <cstdint>
#include <map>
#include <mutex>
#include <random>
#include <tuple>
#include <utility>
#include <vector>

namespace {

constexpr size_t kTableCasesNumber = size_t{1} << 32;

using CaseKey = std::pair<uint16_t, size_t>;
using SparseValueKey = std::tuple<uint16_t, size_t, std::vector<bool>>;

std::map<CaseKey, std::vector<bool>> g_medium_tables;
std::mutex g_medium_tables_mutex;

std::map<SparseValueKey, bool> g_sparse_values;
std::map<CaseKey, std::mt19937> g_sparse_random_generators;
std::mutex g_sparse_values_mutex;

std::vector<bool> InputBits(std::string_view input) {
    std::vector<bool> bits;
    bits.reserve(input.size());
    for (char bit : input) {
        assert(bit == '0' || bit == '1');
        bits.push_back(bit == '1');
    }
    return bits;
}

size_t InputId(std::string_view input) {
    assert(input.size() <= kSolvableTableBitness);

    size_t id = 0;
    for (size_t bit_id = 0; bit_id < input.size(); ++bit_id) {
        assert(input[bit_id] == '0' || input[bit_id] == '1');
        if (input[bit_id] == '1') {
            id |= size_t{1} << bit_id;
        }
    }
    return id;
}

uint64_t SmallTableMask(uint16_t bitness) {
    assert(IsSmallBitness(bitness));
    switch (bitness) {
        case 4: return 0xffffull;
        case 5: return 0xffffffffull;
        case 6: return 0xffffffffffffffffull;
        default: assert(false); return 0;
    }
}

const std::vector<bool>& GetMediumTable(uint16_t bitness, size_t case_id) {
    assert(IsMediumBitness(bitness));
    assert(case_id < MediumBitnessCasesNumber(bitness));

    std::lock_guard<std::mutex> lock(g_medium_tables_mutex);

    const CaseKey key{bitness, case_id};
    auto it = g_medium_tables.find(key);
    if (it == g_medium_tables.end()) {
        it = g_medium_tables.emplace(
            key,
            MediumBitnessTruthTable(bitness, case_id)).first;
    }
    return it->second;
}

TableValueFunction MakeDenseTableValueFunction(uint16_t bitness, size_t case_id) {
    if (IsSmallBitness(bitness)) {
        const uint64_t table = case_id & SmallTableMask(bitness);
        return [bitness, table](std::string_view input) {
            assert(input.size() == bitness);
            return ((table >> InputId(input)) & 1ull) != 0;
        };
    }

    assert(IsMediumBitness(bitness));
    const std::vector<bool>& table = GetMediumTable(bitness, case_id);
    return [bitness, &table](std::string_view input) {
        assert(input.size() == bitness);
        return table[InputId(input)];
    };
}

TableValueFunction MakeSparseTableValueFunction(uint16_t bitness, size_t case_id) {
    assert(bitness > kSolvableTableBitness);
    assert(case_id < bb_table_cases_number(bitness));

    return [bitness, case_id](std::string_view input) {
        assert(input.size() == bitness);

        const std::vector<bool> input_bits = InputBits(input);
        const SparseValueKey key{bitness, case_id, input_bits};
        std::lock_guard<std::mutex> lock(g_sparse_values_mutex);

        auto it = g_sparse_values.find(key);
        if (it == g_sparse_values.end()) {
            const CaseKey case_key{bitness, case_id};
            auto rng_it = g_sparse_random_generators.find(case_key);
            if (rng_it == g_sparse_random_generators.end()) {
                rng_it = g_sparse_random_generators.emplace(
                    case_key,
                    PrepRNG(bitness, case_id)).first;
            }
            it = g_sparse_values.emplace(key, RandomBool(rng_it->second)).first;
        }
        return it->second;
    };
}

}  // namespace

size_t bb_table_cases_number(uint16_t bitness) {
    assert(bitness >= kMinTableBitness);
    return kTableCasesNumber;
}

uint16_t bb_table_solvable_bitness() {
    return kSolvableTableBitness;
}

TableValueFunction MakeTableValueFunction(uint16_t bitness, size_t case_id) {
    assert(bitness >= kMinTableBitness);
    assert(case_id < bb_table_cases_number(bitness));

    if (bitness <= kSolvableTableBitness) {
        return MakeDenseTableValueFunction(bitness, case_id);
    }
    return MakeSparseTableValueFunction(bitness, case_id);
}
