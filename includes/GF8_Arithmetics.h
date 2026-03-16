#ifndef GF8
#define GF8

#include <stdint.h>

// Addition in GF(2^8) = XOR
uint8_t add(uint8_t a, uint8_t b);

// Mult by X
uint8_t xTime(uint8_t a);

// double & add : b = sum_i b_i 2^i => ab = sum_i b_i (a 2^i)
uint8_t mult(uint8_t a, uint8_t b);

// a^e in GF(2^8) square and mult (left to right)
uint8_t GF8_pow(uint8_t a, uint8_t e);

// return a^-1 in GF(2^8) with convention 0^-1 = 0
uint8_t invert(uint8_t a);

#endif