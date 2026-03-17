// il nous faut déjà une fonction qui vient charger les lambda-set dans une var C

#include "../includes/load_sets.h"

LambdaSetCollection load_sets(const char *filename){
    LambdaSetCollection collection;    
    FILE *file = fopen(filename, "r");
    if(!file) {
        return collection;
    }
    
    // On compte d'abord combien de paires on a    
    size_t pair_count = 0;
    char line[256]; // line prend max 256 bytes (on met 256 pour etre larges)
    while(fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Encrypting", 10) == 0) {
            pair_count++;
        }
    }

    // compte le nb de lambda sets
    size_t sets_count = pair_count / LAMBDA_SET_SIZE;
    collection.count = sets_count;
    collection.sets = malloc(sets_count * sizeof(LambdaSet));

    // revient au début du file pour le relire et remplir collection
    rewind(file);
    size_t set_idx = 0;
    size_t pair_idx = 0;
    uint8_t plaintext[16], ciphertext[16];
    char hexstr[33]; // 32 chars + '\0'

    while (fgets(line, sizeof(line), file)) {
        char hexpt[33], hexct[33];
        if (sscanf(line, "Encrypting 0x%32s", hexpt) != 1) continue;
        if (!fgets(line, sizeof(line), file)) break;
        if (sscanf(line, "Results = 0x%32s", hexct) != 1) continue;

        // vérifier terminaisons
        hexpt[32] = '\0'; hexct[32] = '\0';

        // décoder hexpt -> plaintext
        for (int i = 0; i < 16; ++i) {
            unsigned int b;
            if (sscanf(hexpt + 2*i, "%2x", &b) != 1) b = 0;
            plaintext[i] = (uint8_t)b;
        }
        // décoder hexct -> ciphertext
        for (int i = 0; i < 16; ++i) {
            unsigned int b;
            if (sscanf(hexct + 2*i, "%2x", &b) != 1) b = 0;
            ciphertext[i] = (uint8_t)b;
        }

        // remplit la pair
        memcpy(collection.sets[set_idx].pairs[pair_idx].plaintext, plaintext, 16);
        memcpy(collection.sets[set_idx].pairs[pair_idx].ciphertext, ciphertext, 16);

        // passe à la pair d'apres
        pair_idx ++;

        // si on est à la fin d'un lambda-set, passe à celui d'apres
        if (pair_idx >= LAMBDA_SET_SIZE) { 
            pair_idx = 0;
            set_idx++;

            // si on est au dernier lambda set on sort
            if (set_idx >= sets_count) break; 
        }
    } 

    fclose(file);
    return collection;
}

int test_load_sets(){
    LambdaSetCollection collection = load_sets("sets/5_rounds_ciphertexts");
    if (collection.count == 0) {
        fprintf(stderr, "Erreur loading collection \n");
        return 0;
    }
    
    printf("Chargé %zu lambda-sets\n", collection.count);
    
    // Accès à un lambda-set spécifique
    LambdaSet set = collection.sets[0];
    printf("Taille des lambda-sets : %d \n", LAMBDA_SET_SIZE);
    
    // Accès à une paire dans ce lambda-set
    Pair pair = set.pairs[0];
    
    printf("Première paire du premier set:\n");
    printf("Plaintext:  ");
    for (int i = 0; i < 16; i++) printf("%02x", pair.plaintext[i]);
    printf("\nCiphertext: ");
    for (int i = 0; i < 16; i++) printf("%02x", pair.ciphertext[i]);
    printf("\n");
    
    // Libération
    free(collection.sets);
    
    return 1;
}
