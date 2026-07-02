#pragma once

#include <stddef.h>
#include <stdint.h>

#include <functional>
#include <random>
#include <string>
#include <string_view>

bool RandomBool(std::mt19937& rng);
std::mt19937 PrepRNG(uint16_t bitness, size_t case_id);
size_t FullBitId(size_t bit_id, size_t fixed_id);

class FlippingSampler {
public:
    FlippingSampler() = default;
    FlippingSampler(uint16_t bitness, std::string_view input);

    void Reset(uint16_t bitness, std::string_view input);
    std::string input;

    void Fill(
        std::string& value,
        size_t sample_offset,
        size_t fixed_bit_id,
        const std::function<bool(std::string_view)>& evaluate);

private:
    uint16_t bitness_ = 0;
};
