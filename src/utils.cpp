#include "utils.h"

#include <cassert>
#include <cstdint>
#include <cstring>

size_t FullBitId(size_t bit_id, size_t fixed_id) {
    return bit_id < fixed_id ? bit_id : bit_id + 1;
}

bool RandomBool(std::mt19937& rng) {
    return std::uniform_int_distribution<int>(0, 1)(rng) != 0;
}

std::mt19937 PrepRNG(uint16_t bitness, size_t case_id) {
    std::seed_seq seed{
        static_cast<uint32_t>(bitness),
        static_cast<uint32_t>(case_id),
    };
    return std::mt19937(seed);
}

const char* FillRestrictions(
    uint16_t bitness,
    const char* input,
    const std::function<bool(std::string_view)>& evaluate)
{
    assert(input != nullptr);

    thread_local std::string value;
    thread_local std::string sample_input;

    const size_t free_bits = bitness - 1;
    const size_t sample_size = 2 * free_bits + 1;
    const size_t restrictions = bitness * 2;
    const size_t input_stride = restrictions * free_bits;
    const size_t input_size = std::strlen(input);
    assert(input_stride > 0);
    assert(input_size % input_stride == 0);
    const size_t reps = input_size / input_stride;

    value.assign(restrictions * reps * sample_size, '0');
    sample_input.assign(bitness, '0');

    for (size_t fixed_bit_id = 0; fixed_bit_id < bitness; ++fixed_bit_id) {
        for (size_t fixed_bit_value = 0; fixed_bit_value <= 1; ++fixed_bit_value) {
            const size_t restriction_id = fixed_bit_id * 2 + fixed_bit_value;
            sample_input[fixed_bit_id] = static_cast<char>('0' + fixed_bit_value);

            for (size_t rep = 0; rep < reps; ++rep) {
                const size_t input_offset =
                    (restriction_id * reps + rep) * free_bits;
                for (size_t coord = 0; coord < free_bits; ++coord) {
                    sample_input[FullBitId(coord, fixed_bit_id)] =
                        input[input_offset + coord];
                }

                FlippingSampler sampler(bitness, sample_input);
                sampler.Fill(
                    value,
                    (restriction_id * reps + rep) * sample_size,
                    fixed_bit_id,
                    evaluate);
            }
        }
    }

    return value.c_str();
}

FlippingSampler::FlippingSampler(uint16_t bitness, std::string_view input) {
    Reset(bitness, input);
}

void FlippingSampler::Reset(uint16_t bitness, std::string_view input) {
    bitness_ = bitness;
    this->input.assign(input.data(), input.size());
}

void FlippingSampler::Fill(
    std::string& value,
    size_t sample_offset,
    size_t fixed_bit_id,
    const std::function<bool(std::string_view)>& evaluate)
{
    const size_t free_bits = fixed_bit_id < bitness_ ? bitness_ - 1 : bitness_;

    for (size_t coord = 0; coord < free_bits; ++coord) {
        value[sample_offset + coord] = input[FullBitId(coord, fixed_bit_id)];
    }
    value[sample_offset + free_bits] = evaluate({input.data(), bitness_}) ? '1' : '0';
    for (size_t coord = 0; coord < free_bits; ++coord) {
        char& bit = input[FullBitId(coord, fixed_bit_id)];
        bit = bit == '1' ? '0' : '1';
        value[sample_offset + free_bits + 1 + coord] =
            evaluate({input.data(), bitness_}) ? '1' : '0';
        bit = bit == '1' ? '0' : '1';
    }
}
