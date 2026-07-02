#pragma once

#include <stddef.h>
#include <stdint.h>

#include <random>

bool RandomBool(std::mt19937& rng);
std::mt19937 PrepRNG(uint16_t bitness, size_t case_id);
