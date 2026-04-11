// Standard NIST : 
// https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.197.pdf
// On va recoder les primitives d'AES 128 (10 tours classiquement), pour les détails on renvoie à wikipédia ou au papier du NIST 

#include <stdio.h>
#include "aes.h"
#include "GF8_Arithmetics.h"
#include "helpers.h"


// AES


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

/* // Extract RoundKeys for n rounds from words
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
*/

void getRoundKey(const uint32_t words[], int round, key out){
    for(int i = 0; i < 4; ++i){
        fromWord(words[4 * round + i], out + 4 * i);
    }
}

// takes the master key and computes roundkeys
void KeySchedule(key roundkeys[11], key masterkey){
    uint32_t words[44];
    size_t got = KeyExpansion(masterkey, words, 44);

    printf("\nKeySchedule : \n");
    /* Extract round keys */
    for(int r=0;r<=10;r++) {
        printf("K%d : ", r);
        getRoundKey(words, r, roundkeys[r]);
        print_key(roundkeys[r]);
    }
    printf("\n");
}


// invKeySchedule: input roundKey (16 bytes), round index r (0..10) -> output masterKey (16 bytes)
void invKeySchedule(const key round_key, int r, key master_key) {
  // Convert roundKey to words W[0..3]
  uint32_t W[4];
  for (int i=0;i<4;++i) W[i] = toWord(round_key + 4*i);

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
    init_mult_table();

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


    KeySchedule(round_keys, key_master);

    /* Test invKeySchedule */
    key master_key_test;
    invKeySchedule(round_keys[4], 4, master_key_test);
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
