// Standard NIST : 
// https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.197.pdf
// On va recoder les primitives d'AES 128 (10 tours classiquement), pour les détails on renvoie à wikipédia ou au papier du NIST 

#include <stdio.h>
#include "../includes/aes.h"
#include "../includes/GF8_Arithmetics.h"
#include "../includes/helpers.h"


// AES

// sbox precalculed
static const uint8_t sbox[256] = {
  // 256 values...
  0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
  0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
  0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
  0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
  0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
  0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
  0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
  0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
  0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
  0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
  0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
  0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
  0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
  0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
  0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
  0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

// inv sbox precalc
static const uint8_t inv_sbox[256] = {
  // 256 values...
  0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
  0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
  0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
  0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
  0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
  0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
  0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
  0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
  0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
  0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
  0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
  0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
  0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
  0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
  0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
  0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

// Rcon values : used for KeyExpansion 
static const uint32_t Rcon[11] = {
    0x00000000u, // Rcon[0] unused
    0x01000000u,
    0x02000000u,
    0x04000000u,
    0x08000000u,
    0x10000000u,
    0x20000000u,
    0x40000000u,
    0x80000000u,
    0x1B000000u,
    0x36000000u
};


void SubBytes(State s){
    for(int c = 0; c < 4; c++){
        for(int r = 0; r < 4; r++){
            s[c][r] = sbox[s[c][r]];
        }
  }
}

void InvSubBytes(State s){
    for(int c = 0; c < 4; c++) for(int r = 0; r < 4; r++) s[c][r] = inv_sbox[s[c][r]];
}

// Subuint32_t : applies sbox to each byte of a word
static inline uint32_t SubWord(uint32_t w){
    uint8_t b[4]; // b[0] = MSB, b[3] = LSB (même convention que toWord)
    fromWord(w, b); // slice w into 4 bytes put in b
    // applies sbox
    b[0] = sbox[b[0]];
    b[1] = sbox[b[1]];
    b[2] = sbox[b[2]];
    b[3] = sbox[b[3]];
    return toWord(b);  // glue the transformed bytes in 1 word
}

static uint32_t g(uint32_t word, int rcon_index) {
  return SubWord(RotWord(word)) ^ (Rcon[rcon_index]);
}

void ShiftRows(State s){
    State temp; // need copy this cause we cant modify s while browsing it
    for(int c=0;c<4;c++)
        for(int r=0;r<4;r++)
            temp[c][r] = s[c][r];

    for(int r = 0; r < 4; r++){
        for(int c = 0; c < 4; c++){
            s[c][r] = temp[(c + r) % 4][r]; // rotate row r to the left by r
        }
    }
}

void InvShiftRows(State s){
    State temp; // need copy this cause we cant modify s while browsing it
    for(int c=0;c<4;c++)
        for(int r=0;r<4;r++)
            temp[c][r] = s[c][r];
 
    for(int r = 0; r < 4; r++){
        for(int c = 0; c < 4; c++){
        s[c][r] = temp[(c - r + 4) % 4][r]; // rotate row r to the right by r
        }
    }
}

static void MixColumn(uint8_t col[4]){
    uint8_t temp[4];
    for(int i=0;i<4;i++) temp[i] = col[i]; // copie col dans temp
    col[0] = (uint8_t) (mult(0x02, temp[0]) ^ mult(0x03, temp[1]) ^ temp[2] ^ temp[3]);
    col[1] = (uint8_t) (temp[0] ^ mult(0x02, temp[1]) ^ mult(0x03, temp[2]) ^ temp[3]);
    col[2] = (uint8_t) (temp[0] ^ temp[1] ^ mult(0x02, temp[2]) ^ mult(0x03, temp[3]));
    col[3] = (uint8_t) (mult(0x03, temp[0]) ^ temp[1] ^ temp[2] ^ mult(0x02, temp[3]));

}

void MixColumns(State s){
  for(int c = 0; c < 4; c++){
    uint8_t col[4];
    for(int r = 0; r < 4; r++) col[r] = s[c][r]; // just copy the column c in col
    MixColumn(col); // modify col but not the state
    for(int r = 0; r < 4; r++) s[c][r] = col[r]; // modify state
  }
}

static void InvMixColumn(uint8_t col[4]){
    uint8_t temp[4];
    for(int i=0;i<4;i++) temp[i] = col[i]; // copie
    col[0] = (uint8_t) (mult(0x0e, temp[0]) ^ mult(0x0b, temp[1]) ^ mult(0x0d, temp[2]) ^ mult(0x09, temp[3]));
    col[1] = (uint8_t) (mult(0x09, temp[0]) ^ mult(0x0e, temp[1]) ^ mult(0x0b, temp[2]) ^ mult(0x0d, temp[3]));
    col[2] = (uint8_t) (mult(0x0d, temp[0]) ^ mult(0x09, temp[1]) ^ mult(0x0e, temp[2]) ^ mult(0x0b, temp[3]));
    col[3] = (uint8_t) (mult(0x0b, temp[0]) ^ mult(0x0d, temp[1]) ^ mult(0x09, temp[2]) ^ mult(0x0e, temp[3]));
}

void InvMixColumns(State s){
  for(int c = 0; c < 4; c++){
    uint8_t col[4];
    for(int r = 0; r < 4; r++) col[r] = s[c][r]; // just copy the column c in col
    InvMixColumn(col); // modify col but not the state
    for(int r = 0; r < 4; r++) s[c][r] = col[r]; // modify state
  }
}

void AddRoundKey(State s, const key rk){ // rk for RoundKey
  for(int c = 0; c < 4; c++) for(int r = 0; r < 4; r++) s[c][r] ^= rk[c * 4 + r]; // involutif
}

// KeyExpansion for AES 128, input : masterKey (16 uint8_ts = 128b), output : vector of 44 words
/* retourne le nombre de mots écrits (Nwords == 44) */
size_t KeyExpansion(const key masterKey, uint32_t words_out[], size_t words_out_len){
    const int Nk = 4;
    const int Nr = 10;
    const int Nb = 4;
    const int Nwords = Nb * (Nr + 1); /* 44 */

    if (words_out_len < (size_t)Nwords) return 0; /* buffer trop petit */

    /* init words[0..3] = key */
    for(int i = 0; i < Nk; ++i){
        uint8_t temp[4];
        temp[0] = masterKey[4 * i + 0];
        temp[1] = masterKey[4 * i + 1];
        temp[2] = masterKey[4 * i + 2];
        temp[3] = masterKey[4 * i + 3];
        words_out[i] = toWord(temp);
    }

    /* expansion */
    for(int i = Nk; i < Nwords; ++i){
        uint32_t temp = words_out[i - 1];
        if (i % Nk == 0){
            temp = SubWord(RotWord(temp)) ^ Rcon[i / Nk];
        }
        words_out[i] = words_out[i - Nk] ^ temp;
    }

    return (size_t)Nwords;
}

// Extract RoundKeys for n rounds from words
void getRoundKey(const uint32_t words[], int round, key out){
    uint8_t tmp[4];
    for(int i = 0; i < 4; ++i){
        fromWord(words[4 * round + i], tmp);
        out[4 * i + 0] = tmp[0];
        out[4 * i + 1] = tmp[1];
        out[4 * i + 2] = tmp[2];
        out[4 * i + 3] = tmp[3];
    }
}


// invKeySchedule: input roundKey (16 bytes), round index r (0..10) -> output masterKey (16 bytes)
void invKeySchedule(const key round_keys, int r, key master_key) {
  // Convert roundKey to words W[0..3]
  uint32_t W[4];
  for (int i=0;i<4;++i) W[i] = toWord(round_keys + 4*i);

  // We'll reconstruct backwards.
  for (int round = r; round > 0; --round) {
    // Compute previous words prevW[0..3]
    uint32_t prevW[4];
    // prevW[3] = W[3] ^ W[2]
    prevW[3] = W[3] ^ W[2];
    // prevW[2] = W[2] ^ W[1]
    prevW[2] = W[2] ^ W[1];
    // prevW[1] = W[1] ^ W[0]
    prevW[1] = W[1] ^ W[0];
    // prevW[0] = W[0] ^ g(prevW[3], round)   <-- here g uses prevW[3]
    prevW[0] = W[0] ^ g(prevW[3], round);

    // move prevW into W for next iteration
    for (int i=0;i<4;++i) W[i] = prevW[i];
  }

  // Output master key
  for (int i=0;i<4;++i) fromWord(W[i], master_key + 4*i);
}


void AES_Encrypt(State s, const key round_keys[], int rounds){
    AddRoundKey(s, round_keys[0]); /* s modifié in-place */
    for(int r = 1; r <= rounds - 1; ++r){
        SubBytes(s);
        ShiftRows(s);
        MixColumns(s);
        AddRoundKey(s, round_keys[r]);
    }
    /* last round */
    SubBytes(s);
    ShiftRows(s);
    AddRoundKey(s, round_keys[rounds]);
}

/* rounds = 10 */
void AES_Decrypt(State s, const key round_keys[], int rounds){
    AddRoundKey(s, round_keys[rounds]);
    for(int r = rounds - 1; r >= 1; --r){
        InvShiftRows(s);
        InvSubBytes(s);
        AddRoundKey(s, round_keys[r]);
        InvMixColumns(s);
    }
    InvShiftRows(s);
    InvSubBytes(s);
    AddRoundKey(s, round_keys[0]);
}

// f prend en entrée une clé de chaîne k_i et la chiffre avec aes (en gérant les types) -> modifie k_i (lors du stockage il faudra faire une copie)
void f(key ki, const key round_keys[], int rounds){
    State s;
    bytes_to_state(ki, s);
    AES_Encrypt(s, round_keys, rounds);
    state_to_bytes(s, ki);
}

// test que l'aes chiffre et déchiffre comme il faut
int test_aes(){

    key round_keys[11]; // round_keys est calculé dans le test aes

    /* NIST AES-128 test vector (FIPS 197 A.1) */
    static uint8_t key_master[16] = {
        0x2b,0x7e,0x15,0x16, 0x28,0xae,0xd2,0xa6, 0xab,0xf7,0x15,0x88, 0x09,0xcf,0x4f,0x3c
    };

    static uint8_t plaintext[16] = {
        0x32,0x43,0xf6,0xa8, 0x88,0x5a,0x30,0x8d, 0x31,0x31,0x98,0xa2, 0xe0,0x37,0x07,0x34
    };

    static uint8_t expected_cipher[16] = {
        0x39,0x25,0x84,0x1d, 0x02,0xdc,0x09,0xfb, 0xdc,0x11,0x85,0x97, 0x19,0x6a,0x0b,0x32
    };

    /* Compute expanded key words */
    uint32_t words[44];
    size_t got = KeyExpansion(key_master, words, 44);
    if(got != 44){
        fprintf(stderr, "KeyExpansion failed (got %zu words)\n", got);
        return 1;
    }

    /* Extract round keys */
    for(int r=0;r<=10;r++) getRoundKey(words, r, round_keys[r]);

    /* Test invKeySchedule */
    key master_key_test;
    invKeySchedule(round_keys[10], 10, master_key_test);
    if(key_equal(master_key_test, key_master)){
        printf("invKeySchedule OK \n");
    } else{
        printf("Problem with invKeySchedule \n");
        return 4;
    }

    /* Prepare state from plaintext */
    State s;
    bytes_to_state(plaintext, s);

    /* Encrypt */
    AES_Encrypt(s, round_keys, 10);

    uint8_t cipher[16];
    state_to_bytes(s, cipher);

    printf("Plaintext:        "); print_hex(plaintext, 16);
    printf("Expected cipher:  "); print_hex(expected_cipher, 16);
    printf("Computed cipher:  "); print_hex(cipher, 16);

    if(memcmp(cipher, expected_cipher, 16) != 0){
        fprintf(stderr, "Encryption mismatch\n");
        return 2;
    } else {
        printf("Encryption OK\n");
    }

    /* Decrypt to verify roundtrip */
    State s2;
    bytes_to_state(cipher, s2);
    AES_Decrypt(s2, round_keys, 10);
    uint8_t recovered[16];
    state_to_bytes(s2, recovered);

    printf("Recovered plain:  "); print_hex(recovered, 16);
    if(memcmp(recovered, plaintext, 16) != 0){
        fprintf(stderr, "Decryption mismatch\n");
        return 3;
    } else {
        printf("Decryption OK\n");
    }
    return 0;
}
