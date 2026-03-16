#ifndef helpers_h
#define helpers_h

#include <stdint.h>
#include <stddef.h>

//using byte = uint8_t;
//using word = uint32_t; // 1 word = 4 bytes here
typedef uint8_t key[16]; // in AES 128 keys (master key & round keys) are 16 bytes (we call it bytes here)
typedef uint8_t State[4][4]; // column-major: state[col][row]

int key_equal(const key a, const key b);

void print_hex(const uint8_t *b, size_t n);

void print_key(const key k);

// [16] to [4][4] with State[col][row]
void bytes_to_state(const key in, State s);

// [4][4] with State[col][row] to [16]
void state_to_bytes(const State s, key out);

// from 4 bytes to word (= uint32)
uint32_t toWord(const uint8_t b[4]);

// from word to 4 bytes
void fromWord(uint32_t w, uint8_t out[4]);

// RotWord : rotate left by 1 byte
uint32_t RotWord(uint32_t w);

#endif
