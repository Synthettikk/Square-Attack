#ifndef aes_h
#define aes_h

#include <stdint.h>
#include <string.h>

//using byte = uint8_t;
//using word = uint32_t; // 1 word = 4 bytes here
typedef uint8_t key[16]; // in AES 128 keys (master key & round keys) are 16 bytes
typedef uint8_t State[4][4]; // column-major: state[col][row]

// KeyExpansion for AES 128, input : masterKey (16 uint8_ts = 128b), output : vector of 44 words
/* retourne le nombre de mots écrits (Nwords == 44) */
size_t KeyExpansion(const key masterKey, uint32_t words_out[], size_t words_out_len);

// Extract RoundKeys for n rounds from words
void getRoundKey(const uint32_t words[], int round, key out);

uint8_t InvSubByte(uint8_t byte);
void SubBytes(State s);
void InvSubBytes(State s);
void ShiftRows(State s);
void InvShiftRows(State s);
void MixColumns(State s);
void InvMixColumns(State s);
void AddRoundKey(State s, const key rk); // rk for RoundKey

// takes the master key and computes roundkeys
void KeySchedule(key roundkeys[11], key masterkey);

// invKeySchedule: input roundKey (16 bytes), round index r (0..10) -> output masterKey (16 bytes)
void invKeySchedule(const key round_key, int r, key master_key);

void AES_Encrypt(State s, const key round_keys[], int rounds);
void AES_Decrypt(State s, const key round_keys[], int rounds);

// f prend en entrée une clé de chaîne k_i et la chiffre avec aes (en gérant les types) -> modifie k_i (lors du stockage il faudra faire une copie)
void f(key ki, const key round_keys[], int rounds);

// test que l'aes chiffre et déchiffre comme il faut
int test_aes();

#endif // aes_h