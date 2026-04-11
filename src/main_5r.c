// attaque sur aes 5 tours

#include "aes.h"
#include "load_sets.h"
#include "helpers.h"
#include "GF8_Arithmetics.h"
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

typedef struct { uint8_t c; uint8_t r; } Pos;


void print_progress(int col, int guess5_byte1, int guess5_byte2) {
    float progress = 25.0f * col + (guess5_byte1 / 256.0f) * 25.0f + (guess5_byte2 / 65536.0f) * 25.0f;
    int bar_width = 40;
    int filled = (int)(progress / 100.0f * bar_width);
    printf("\r[Col %d] [", col);
    for (int i = 0; i < bar_width; i++) {
        printf(i < filled ? "#" : "-");
    }
    printf("] %.1f%%", progress);
    fflush(stdout);
}


// inverse les tours 4 et 5 d'aes 5 tours (que la col nécessaire) et return l'indice 0 de la col
static inline uint8_t invert_rounds(
    uint8_t col_state[4],
    uint8_t guess4,
    const Pos (*positions)[4],
    int col,
    const State key5
) {
    // Inv ARK5
    col_state[0] ^= key5[positions[col][0].c][positions[col][0].r];
    col_state[1] ^= key5[positions[col][1].c][positions[col][1].r];
    col_state[2] ^= key5[positions[col][2].c][positions[col][2].r];
    col_state[3] ^= key5[positions[col][3].c][positions[col][3].r];
    
    // invSB_5
    col_state[0] = InvSubByte(col_state[0]);
    col_state[1] = InvSubByte(col_state[1]);
    col_state[2] = InvSubByte(col_state[2]);
    col_state[3] = InvSubByte(col_state[3]);
    
    // invMC
    InvMixColumn(col_state);
    
    // Inv ARK4
    col_state[0] ^= guess4;
    
    // invSB_4
    col_state[0] = InvSubByte(col_state[0]);
    
    return col_state[0];
}

// pour accumuler sur un lambda-set : return true si acc == 0
static inline bool check_lambda_set(
    uint8_t guess4,
    const LambdaSet *set,
    const Pos (*positions)[4],
    int col,
    const State key5
) {
    uint8_t acc = 0;
    
    for(int cipher_idx = 0; cipher_idx < 256; cipher_idx += 1) {
        uint8_t col_state[4];
        // copie dans col state les 4 octets ciblés
        col_state[0] = set->pairs[cipher_idx].ciphertext_state[positions[col][0].c][positions[col][0].r];
        col_state[1] = set->pairs[cipher_idx].ciphertext_state[positions[col][1].c][positions[col][1].r];
        col_state[2] = set->pairs[cipher_idx].ciphertext_state[positions[col][2].c][positions[col][2].r];
        col_state[3] = set->pairs[cipher_idx].ciphertext_state[positions[col][3].c][positions[col][3].r];
        
        acc ^= invert_rounds(col_state, guess4, positions, col, key5);
    }
    
    return acc == 0;  // équilibré si acc == 0
}

// attaque en connaissant 3 octet de la col
bool attack5r_1byte(State key5, int col, const LambdaSetCollection *collection, const Pos (*positions)[4]) {

    bool is_key;

    // pour chaque guess de clé, on met direct l'octet dans la clé à la bonne pos
    for(int guess5_byte1 = 0; guess5_byte1 < 256; guess5_byte1 += 1) {
        key5[positions[col][0].c][positions[col][0].r] = guess5_byte1;
        
        //print_progress(col, guess5_byte1, 0); // super lent
        
        for(int guess4 = 0; guess4 < 256; guess4 += 1) {

            is_key = true; // reset à true pour chaque guess

            for(int set = 0; set < collection->count; set += 1) {
                if (__builtin_expect(!check_lambda_set(guess4, &collection->sets[set], positions, col, key5), 1)) {
                    is_key = false;
                    break;
                }
            }
            
            // si is_key est resté vrai apres les 6 lambda-sets, la proba que ce soit un faux est TRES faible
            if (is_key) {
                return true;
            }
        } // fin du guess4 
    } // fin du guess5_byte1
    
    return false;
}


// attaque en connaissant 2 octets de la col
bool attack5r_2bytes(State key5, int col, const LambdaSetCollection *collection, const Pos (*positions)[4]) {

    bool is_key;

    // pour chaque guess de clé, on met direct l'octet dans la clé à la bonne pos
    for(int guess5_byte1 = 0; guess5_byte1 < 256; guess5_byte1 += 1) {
        key5[positions[col][0].c][positions[col][0].r] = guess5_byte1;
        
        for(int guess5_byte2 = 0; guess5_byte2 < 256; guess5_byte2 += 1) {
            key5[positions[col][1].c][positions[col][1].r] = guess5_byte2;

            print_progress(col, guess5_byte1, guess5_byte2); // super lent
            
            for(int guess4 = 0; guess4 < 256; guess4 += 1) {

                is_key = true; // reset à true pour chaque guess

                for(int set = 0; set < collection->count; set += 1) {
                    if (__builtin_expect(!check_lambda_set(guess4, &collection->sets[set], positions, col, key5), 1)) {
                        is_key = false;
                        break;
                    }
                }
                
                // si is_key est resté vrai apres les 6 lambda-sets, la proba que ce soit un faux est TRES faible
                if (is_key) {
                    return true;
                }
            } // fin du guess4
        } // fin du guess5_byte2   
    } // fin du guess5_byte1
    
    return false;
}


// attaque en connaissant 1 octet de la col
bool attack5r_3bytes(State key5, int col, const LambdaSetCollection *collection, const Pos (*positions)[4]) {

    bool is_key;

    // pour chaque guess de clé, on met direct l'octet dans la clé à la bonne pos
    for(int guess5_byte1 = 0; guess5_byte1 < 256; guess5_byte1 += 1) {
        key5[positions[col][0].c][positions[col][0].r] = guess5_byte1;
        
        for(int guess5_byte2 = 0; guess5_byte2 < 256; guess5_byte2 += 1) {
            key5[positions[col][1].c][positions[col][1].r] = guess5_byte2;

            print_progress(col, guess5_byte1, guess5_byte2); // super lent

            for(int guess5_byte3 = 0; guess5_byte3 < 256; guess5_byte3 += 1) {
                key5[positions[col][2].c][positions[col][2].r] = guess5_byte2;
            
                for(int guess4 = 0; guess4 < 256; guess4 += 1) {

                    is_key = true; // reset à true pour chaque guess

                    for(int set = 0; set < collection->count; set += 1) {
                        if (__builtin_expect(!check_lambda_set(guess4, &collection->sets[set], positions, col, key5), 1)) {
                            is_key = false;
                            break;
                        }
                    }
                    
                    // si is_key est resté vrai apres les 6 lambda-sets, la proba que ce soit un faux est TRES faible
                    if (is_key) {
                        return true;
                    }
                } // fin du guess4
            } // fin du guess5_byte3
        } // fin du guess5_byte2   
    } // fin du guess5_byte1
    
    return false;
}


// attaque en connaissant rien
bool attack5r_4bytes(State key5, int col, const LambdaSetCollection *collection, const Pos (*positions)[4]) {

    bool is_key;

    // pour chaque guess de clé, on met direct l'octet dans la clé à la bonne pos
    for(int guess5_byte1 = 0; guess5_byte1 < 256; guess5_byte1 += 1) {
        key5[positions[col][0].c][positions[col][0].r] = guess5_byte1;
        
        for(int guess5_byte2 = 0; guess5_byte2 < 256; guess5_byte2 += 1) {
            key5[positions[col][1].c][positions[col][1].r] = guess5_byte2;

            print_progress(col, guess5_byte1, guess5_byte2); // super lent

            for(int guess5_byte3 = 0; guess5_byte3 < 256; guess5_byte3 += 1) {
                key5[positions[col][2].c][positions[col][2].r] = guess5_byte2;

                for(int guess5_byte4 = 0; guess5_byte4 < 256; guess5_byte4 += 1) {
                    key5[positions[col][3].c][positions[col][3].r] = guess5_byte2;
            
                    for(int guess4 = 0; guess4 < 256; guess4 += 1) {

                        is_key = true; // reset à true pour chaque guess

                        for(int set = 0; set < collection->count; set += 1) {
                            if (__builtin_expect(!check_lambda_set(guess4, &collection->sets[set], positions, col, key5), 1)) {
                                is_key = false;
                                break;
                            }
                        }
                        
                        // si is_key est resté vrai apres les 6 lambda-sets, la proba que ce soit un faux est TRES faible
                        if (is_key) {
                            return true;
                        }
                    } // fin du guess4
                } // fin du guess5_byte4
            } // fin du guess5_byte3
        } // fin du guess5_byte2   
    } // fin du guess5_byte1
    
    return false;
}



int main(int argc, char *argv[]){

    char *set;

    // Vérif qu'un argument (chemin) a été passé    
    if (argc < 2) {        
        fprintf(stderr, "Usage: %s <chemin_des_sets>\n", argv[0]);
        printf("Chemin par défaut : sets/5r_sets\n");  
        set = "sets/5r_sets";
    } else{
        set = argv[1];
    }

    // Vérif que le chemin existe et est lisible    
    if (access(set, F_OK) == -1) {        
        fprintf(stderr, "Erreur: le fichier '%s' n'existe pas\n", set);        
        return 1;    
    }

    init_mult_table();

    LambdaSetCollection collection = load_sets("sets/5r_sets");
    State key5_state; // sous forme de state
    // 03 12 07 13 09 8d d1 16 22 17 0f 2c 33 09 0d 27
    /*
    key key5 = {0x03, 0x00, 0x04, 0x02, 0x00, 0x01, 0x05, 0x00,
                0x04, 0x07, 0x02, 0x01, 0x00, 0x05, 0x00, 0x07};
    // on lui passe la vraie clé car on a pas la puissance de calcul de retrouver 4 octets de K5
    key master_key = {0x5b, 0x09, 0xf7, 0x68, 0xbb, 0xc2, 0x22, 0xe4,
                0x81, 0x16, 0x30, 0xa7, 0xfe, 0x69, 0xc7, 0x65};
    */

    key key5 = {0x6d, 0xdf, 0x34, 0x43, 0x91, 0xbb, 0xe2, 0xe7,
                0xfa, 0xed, 0x79, 0xc3, 0x66, 0x32, 0xed, 0x82};
                
    key master_key = {0xb8, 0x0d, 0xcc, 0x7d, 0x9b, 0x8d, 0xd1, 0x57,
                0x2a, 0x0c, 0x0f, 0x9c, 0x33, 0xa9, 0xd9, 0x29};
    

   bytes_to_state(key5, key5_state);

    /* 
    POSITIONS : preshot des positions où appliquer ARK5 en fct de la colonne ciblée
    ===========================================================================
    on focus tj la premiere ligne -> [x][0]
    apres le SR4 ca bouge pas 
    apres MC4 toute la colonne devient impliquée
    apres SR5 -> [(x - y) % 4][y] 
            y =    0        1      2      3
    pour x = 0 : [0][0], [3][1] [2][2], [1][3]
    pour x = 1 : [1][0], [0][1], [3][2], [2][3] 
    pour x = 2 : [2][0], [1][1], [0][2], [3][3]
    pour x = 3 : [3][0], [2][1], [1][2], [0][3]*/
    const Pos positions[4][4] = {
        // pour x = 0 : [0][0], [3][1], [2][2], [1][3]
        { {0,0}, {3,1}, {2,2}, {1,3} },

        // pour x = 1 : [1][0], [0][1], [3][2], [2][3]
        { {1,0}, {0,1}, {3,2}, {2,3} },

        // pour x = 2 : [2][0], [1][1], [0][2], {3,3}
        { {2,0}, {1,1}, {0,2}, {3,3} },

        // pour x = 3 : [3][0], [2][1], [1][2], [0][3}
        { {3,0}, {2,1}, {1,2}, {0,3} }
    };


    // attaque et mesure le temps 
    printf("Lancement de l'attaque...\n");

    // note pour l'utilisateur : on fait l'attaque que sur 2 octets pour chaque colonne et on donne les autres, sinon cest trop long sur cpu
    printf("Attention : c'est une preuve de concept : on n'attaque que sur deux octets par colonne (trop long sur CPU de faire la vraie attaque, voir le code CUDA)\n");

    clock_t start, end;
    start = clock();

    // pour chaque colonne du state ; la ligne est tj 0 : le MC implique ensuite toute la col
    for(int col = 0; col < 4; col++){ // parallélisable sur 4 machines (les 4 attaques sont indépendantes)
        // recupere la clé de tour 5 et la met dans key5_state
        attack5r_2bytes(key5_state, col, &collection, positions);
    }

    end = clock();

    print_progress(4, 0, 0); // pour que la barre de progression se mette à 100%
    printf("\nClé du tour 5 trouvée : ");
    print_state(key5_state);
    printf("Durée de l'attaque : %.4f secondes \n", (double)(end-start)/CLOCKS_PER_SEC);

    state_to_bytes(key5_state, key5);

    // remonte K5 à master_key
    invKeySchedule(key5, 5, master_key);
    printf("Clé maître trouvée : ");
    print_key(master_key);
    printf("\n");

    // test de la master key

    printf("Test de la masterkey... \n");

    key round_keys[11];
    KeySchedule(round_keys, master_key);

    State text; // to encipher
    bytes_to_state(collection.sets[0].pairs[1].plaintext, text); // text <- plain
    printf(" Plaintext : ");
    print_state(text);

    State expected_cipher;
    memcpy(expected_cipher, collection.sets[0].pairs[1].ciphertext_state, sizeof(State));
    printf("  Expected : ");
    print_state(expected_cipher);

    // encipher text : text <- cipher
    AES_Encrypt(text,round_keys, 5);

    printf("Ciphertext : ");
    print_state(text);
    printf("\n");

    // verif egalité
    key expected;
    key ciphertext;
    state_to_bytes(expected_cipher, expected); state_to_bytes(text, ciphertext);
    if(key_equal(expected, ciphertext)){
        printf("Vérification réussie !\nLa clé maître est :");
        print_key(master_key);
    } else{
        printf("Échec. \nLa clé maître n'est PAS :");
        print_key(master_key);
    }


    free(collection.sets);
    return 0;
}
