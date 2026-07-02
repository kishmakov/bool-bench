#include "utils.h"

#include <cstdint>

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
