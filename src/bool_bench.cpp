#include "aig.h"
#include "bool_bench.h"
#include "table.h"
#include "tree.h"
#include "utils.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <string_view>

// API

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
                    return EvaluateTreeCase(bitness, case_id, point);
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
