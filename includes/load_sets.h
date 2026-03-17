#ifndef LOAD_SETS
#define LOAD_SETS

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LAMBDA_SET_SIZE 256

typedef struct { 
    uint8_t plaintext[16];
    uint8_t ciphertext[16];
} Pair;

typedef struct { 
    Pair pairs[LAMBDA_SET_SIZE];  // Exactement 256 paires
} LambdaSet;

typedef struct { 
    LambdaSet *sets;
    size_t count;
} LambdaSetCollection;

// charge tous les lambda set d'un fichier .txt et renvoie la collection
LambdaSetCollection load_sets(const char *filename);

// renvoie 1 si ok, 0 sinon
int test_load_sets();

#endif // LOAD_SETS