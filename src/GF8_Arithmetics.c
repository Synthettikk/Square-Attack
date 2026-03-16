// GF(2^8) arithmetics for AES

// In the AES standard, F_2^8 = F_2[X]/(X^8 + X^4 + X^3 + X + 1)

#include "../includes/GF8_Arithmetics.h"

static const uint16_t aesPoly = 0x11B; // 0x11B = 100011011
static const uint8_t mod = 0x1B; // 0x1B = 00011011 = X^4 + X^3 + X + 1
static const uint8_t strongBit = 0x80; // 0x80 = 1000 0000

// Addition in GF(2^8) = XOR
uint8_t add(uint8_t a, uint8_t b){
    return a ^ b;
}

uint8_t xTime(uint8_t a){
    // auto Xa = a << 1; // mult a by X is like shifting the bits
    return (uint8_t) (a << 1) ^ (a & strongBit ? mod : 0x00); // if a has strong bit = 1, reduce Xa mod remainder
}

// double & add : b = sum_i b_i 2^i => ab = sum_i b_i (a 2^i)
uint8_t mult(uint8_t a, uint8_t b){
    uint8_t result = 0;
    uint8_t base = a; // a 2^i (begin with i = 0)
    while(b){
        if(b & 1) result = add(result, base);
        base = xTime(base); // mult by X is like shifting bit = mult by 2
        b >>= 1; // next bit
    }
    return result;
}

// a^e in GF(2^8) square and mult (left to right)
uint8_t GF8_pow(uint8_t a, uint8_t e){
    uint8_t result = 1;
    uint8_t base = a;
    while(e){
        if(e & 1) result = mult(result, base); // mult
        base = mult(base, base); //square
        e >>= 1;
    }
    return result;
}

// return a^-1 in GF(2^8) with convention 0^-1 = 0
uint8_t invert(uint8_t a){
    if (a == 0) return 0;
    return GF8_pow(a, 254); // a^254 = a^(254 - 255) = a^-1 by Fermat
}