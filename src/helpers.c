// Helpers for AES

#include "../includes/helpers.h"
#include <string.h> // for memcmp
#include <stdio.h> // for printf


void print_hex(const uint8_t *b, size_t n){
    for(size_t i=0;i<n;i++) printf("%02x", b[i]);
    printf("\n");
}

/* return true (1) iff keys are equal, else false (0) */
int key_equal(const key a, const key b){
    return memcmp(a, b, 16) == 0;
}

void print_key(const key k){
    for (int b=0;b<16;++b) printf(" %02x", k[b]);
    printf("\n");
}

void print_state(const State state){
  for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) printf(" %02x", state[c][r]);
  printf("\n");
}

// [16] to [4][4] with State[col][row]
void bytes_to_state(const key in, State s){
    for(int c=0;c<4;c++)
      for(int r=0;r<4;r++)
        s[c][r] = in[c*4 + r];
}

// [4][4] with State[col][row] to [16]
void state_to_bytes(const State s, key out) {
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
            out[col*4 + row] = s[col][row];
}

// from 4 bytes to word (= uint32)
inline uint32_t toWord(const uint8_t b[4]){
  return ((uint32_t)(b[0]) << 24) |
          ((uint32_t)(b[1]) << 16) |
          ((uint32_t)(b[2]) << 8)  |
          ((uint32_t)(b[3]));
}

// from word to 4 bytes
inline void fromWord(uint32_t w, uint8_t out[4]){
  out[0] = (w >> 24) & 0xff; // shift to place byte 24-31 at bottom and &0xff isolate it : its and bitwise with 11111111
  out[1] = (w >> 16) & 0xff; // example : 0x12345678 >> 16 = 0x56781234
  out[2] = (w >> 8) & 0xff; // example : 0x56781234 & 0xff = 0x34
  out[3] = w & 0xff;
}

// RotWord : rotate left by 1 byte
uint32_t RotWord(uint32_t w){
  return (w << 8) | (w >> 24); // ex : w = 0x11223344 -> 0x22334411
}

