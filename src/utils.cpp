#include "utils.h"

#include <cstdint>

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
