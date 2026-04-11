#ifndef GF8
#define GF8

#include <stdint.h>

// Précalcule mult_table une fois au démarrage, déclarée 1 seule fois dans le fichier source 
// compile avec gcc pour le code C
extern uint8_t mult_table[256][256]; // tout le temps def (cpu)

#ifdef __CUDA_ARCH__
    // compile avec nvcc pour le code CUDA
    extern __device__ uint8_t d_mult_table[256][256]; // def qu'avec cuda
#endif


// Addition in GF(2^8) = XOR
uint8_t add(uint8_t a, uint8_t b);

static inline uint8_t xTime(uint8_t a) {
    return (uint8_t)((a << 1) ^ (a & 0x80 ? 0x1b : 0x00));
}

// double & add : b = sum_i b_i 2^i => ab = sum_i b_i (a 2^i)
uint8_t mult_slow(uint8_t a, uint8_t b);

// for mult precalc
void init_mult_table();

static inline uint8_t mult(uint8_t a, uint8_t b) {
    return mult_table[a][b];
}

/*
// double & add : b = sum_i b_i 2^i => ab = sum_i b_i (a 2^i)
uint8_t mult(uint8_t a, uint8_t b);
*/

// a^e in GF(2^8) square and mult (left to right)
uint8_t GF8_pow(uint8_t a, uint8_t e);

// return a^-1 in GF(2^8) with convention 0^-1 = 0
uint8_t invert(uint8_t a);

#endif