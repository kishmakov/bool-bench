#pragma once

#include <stdint.h>
#include <stddef.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

size_t bb_gen_cases_number(uint16_t bitness);

// Number of nodes or depth for given case
size_t bb_gen_nodes(uint16_t bitness, size_t case_id);
size_t bb_gen_depth(uint16_t bitness, size_t case_id);

// Computes value of the function at input and all variations of input with one
// bit flipped.
// Return 0/1 string of length 2 * bitness + 1: input [bitness bits] +
// f(input) [1 bit] + f(input with flipped i-th bit) [1 x bitness bits]
const char* bb_gen_value(uint16_t bitness, size_t case_id, const char* input);

// Computes all restriction sample points for one rep as ASCII 0/1 bytes
const char* bb_case_restrictions(uint16_t bitness, size_t case_id, size_t rep);

// Returns sorted newline-separated circuit set names discovered in circuits/
const char* bb_circuit_sets();

// Returns sorted newline-separated circuit case names for a set
const char* bb_circuit_cases(const char* set_name);

size_t bb_circuit_inputs(const char* set_name, const char* case_name);
size_t bb_circuit_outputs(const char* set_name, const char* case_name);

// Computes value samples for an AIG circuit.
// For sequential circuits, input_state contains primary inputs followed by latch state bits.
// Return length is N + M * (N + 1), where N = inputs and M = outputs:
// input_state + outputs(input_state) + outputs(input_state with flipped i-th bit).
const char* bb_circuit_value(const char* set_name, const char* case_name, const char* input_state);

#ifdef __cplusplus
}
#endif
