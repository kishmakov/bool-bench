#include "bool_bench.h"
#include "decision_tree.h"
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
#include <tuple>
#include <utility>
#include <vector>

namespace {

constexpr size_t kTableCasesNumber = size_t{1} << 32;

constexpr uint16_t kMinSmallBitness = 4;
constexpr uint16_t kMaxSmallBitness = 6;

constexpr uint16_t kMinMediumBitness = 7;
constexpr uint16_t kMaxMediumBitness = 16;


using CaseKey = std::pair<uint16_t, size_t>;
using SparseValueKey = std::tuple<uint16_t, size_t, std::vector<bool>>;
using TableValueFunction = std::function<bool(std::string_view)>;

std::map<CaseKey, std::vector<bool>> g_medium_tables;
std::mutex g_medium_tables_mutex;

std::map<SparseValueKey, bool> g_sparse_values;
std::map<CaseKey, std::mt19937> g_sparse_random_generators;
std::mutex g_sparse_values_mutex;

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

bool IsMediumBitness(uint16_t bitness) {
    return kMinMediumBitness <= bitness && bitness <= kMaxMediumBitness;
}

size_t MediumBitnessCasesNumber(uint16_t bitness) {
    assert(IsMediumBitness(bitness));
    return kTableCasesNumber;
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

std::vector<bool> CharsToBits(std::string_view input) {
    std::vector<bool> bits;
    bits.reserve(input.size());
    for (char bit : input) {
        assert(bit == '0' || bit == '1');
        bits.push_back(bit == '1');
    }
    return bits;
}

size_t CharsToNum(std::string_view input) {
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

std::vector<bool> SmallTableVector(uint16_t bitness, size_t case_id) {
    assert(IsSmallBitness(bitness));
    assert(case_id < SmallBitnessCasesNumber(bitness));

    std::vector<bool> table(size_t{1} << bitness);
    for (size_t input_id = 0; input_id < table.size(); ++input_id) {
        table[input_id] = ((case_id >> input_id) & 1ull) != 0;
    }
    return table;
}

std::vector<bool> SolvableTableVector(uint16_t bitness, size_t case_id) {
    assert(bitness >= kMinTableBitness && bitness <= kSolvableTableBitness);
    assert(case_id < bb_table_cases_number(bitness));

    if (IsSmallBitness(bitness)) {
        return SmallTableVector(bitness, case_id);
    }
    return GetMediumTable(bitness, case_id);
}

TableValueFunction MakeSmallTableValueFunction(uint16_t bitness, size_t case_id) {
    assert(IsSmallBitness(bitness));
    assert(case_id < SmallBitnessCasesNumber(bitness));

    return [bitness, case_id](std::string_view input) {
        assert(input.size() == bitness);
        return ((case_id >> CharsToNum(input)) & 1ull) != 0;
    };
}

TableValueFunction MakeDenseTableValueFunction(uint16_t bitness, size_t case_id) {
    assert(IsMediumBitness(bitness));
    const std::vector<bool>& table = GetMediumTable(bitness, case_id);
    return [bitness, &table](std::string_view input) {
        assert(input.size() == bitness);
        return table[CharsToNum(input)];
    };
}

TableValueFunction MakeSparseTableValueFunction(uint16_t bitness, size_t case_id) {
    assert(bitness > kSolvableTableBitness);
    assert(case_id < bb_table_cases_number(bitness));

    return [bitness, case_id](std::string_view input) {
        assert(input.size() == bitness);

        const std::vector<bool> input_bits = CharsToBits(input);
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

TableValueFunction MakeTableValueFunction(uint16_t bitness, size_t case_id) {
    assert(bitness >= kMinTableBitness && bitness <= kMaxTableBitness);
    assert(case_id < bb_table_cases_number(bitness));

    if (IsSmallBitness(bitness)) {
        return MakeSmallTableValueFunction(bitness, case_id);
    }

    if (IsMediumBitness(bitness)) {
        return MakeDenseTableValueFunction(bitness, case_id);
    }

    return MakeSparseTableValueFunction(bitness, case_id);
}

}  // namespace

size_t bb_table_cases_number(uint16_t bitness) {
    assert(bitness >= kMinTableBitness && bitness <= kMaxTableBitness);
    if (IsSmallBitness(bitness)) {
        return SmallBitnessCasesNumber(bitness);
    }
    if (IsMediumBitness(bitness)) {
        return MediumBitnessCasesNumber(bitness);
    }
    return kTableCasesNumber;
}

uint16_t bb_table_solvable_bitness() {
    return kSolvableTableBitness;
}

const char* bb_table_value(uint16_t bitness, size_t case_id, const char* input) {
    assert(bitness >= kMinTableBitness && bitness <= kMaxTableBitness);
    assert(case_id < bb_table_cases_number(bitness));
    assert(input != nullptr);
    assert(std::strlen(input) >= bitness);

    thread_local std::string value;
    thread_local FlippingSampler sampler;

    value.assign(2 * bitness + 1, '0');
    sampler.Reset(bitness, {input, bitness});

    const TableValueFunction evaluate = MakeTableValueFunction(bitness, case_id);
    sampler.Fill(
        value,
        /*sample_offset=*/0,
        bitness,
        evaluate);

    return value.c_str();
}

void bb_table_value_tensor(
    uint16_t bitness,
    const size_t* case_ids,
    size_t cases,
    const char* packed_inputs,
    size_t reps,
    float* out)
{
    assert(bitness >= kMinTableBitness && bitness <= kMaxTableBitness);
    assert(case_ids != nullptr);
    assert(packed_inputs != nullptr);
    assert(out != nullptr);

    const size_t sample_size = 2 * bitness + 1;
    thread_local FlippingSampler sampler;

    for (size_t case_index = 0; case_index < cases; ++case_index) {
        const size_t case_id = case_ids[case_index];
        assert(case_id < bb_table_cases_number(bitness));

        const TableValueFunction evaluate = MakeTableValueFunction(bitness, case_id);

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

const char* bb_table_restrictions(uint16_t bitness, size_t case_id, const char* input) {
    assert(bitness >= kMinTableBitness && bitness <= kMaxTableBitness);
    assert(case_id < bb_table_cases_number(bitness));

    const TableValueFunction evaluate = MakeTableValueFunction(bitness, case_id);
    return FillRestrictions(bitness, input, evaluate);
}

size_t bb_table_nodes(uint16_t bitness, size_t case_id) {
    assert(bitness >= kMinTableBitness && bitness <= kSolvableTableBitness);
    const std::vector<bool> table = SolvableTableVector(bitness, case_id);
    return SolveForSize(bitness, table);
}

size_t bb_table_depth(uint16_t bitness, size_t case_id) {
    assert(bitness >= kMinTableBitness && bitness <= kSolvableTableBitness);
    const std::vector<bool> table = SolvableTableVector(bitness, case_id);
    return SolveForDepth(bitness, table);
}
