/*
Pour générer des lambda-sets
Mais nécessite d'avoir la clé pour utiliser notre AES
Ou alors nécessite un AES type embarqué avec une clé embarquée que l'on ne connaitrait pas
pour pouvoir chiffer les sets
*/

#include "gen_sets.h"
#include "aes.h"
#include "helpers.h"
#include <stdio.h>

#define num_sets_4r 3
#define num_sets_5r 6

/* genere num_sets_4r sets (besoin que de 3 pour l'attaque sur 4 tours)  */
void gen_sets_4r(key master_key){
    FILE *fichier = fopen("sets/4r_sets", "w");
    if (fichier == NULL) {
        printf("Erreur : impossible d'ouvrir le fichier 4r_sets\n");
        return;
    }
    char hex_str[2 + (16 * 2) + 1];  // "0x" + 32 caractères + '\0'
    State s;
    uint8_t bytes[16];

    key roundkeys[11];
    KeySchedule(roundkeys, master_key);
    // fflush(stdout);

    for(int set = 0; set < num_sets_4r; set ++){
        uint8_t plaintext[16] = {0};
        for(int pair = 0; pair < 256; pair++){

            // genere le clair sous forme de hex str
            plaintext[set] = (uint8_t)pair;
            bytes_to_hex_string(plaintext, 16, hex_str);


            // print le clair dans le file
            fprintf(fichier, "Encrypting %s\n", hex_str);

            // convertit le clair en state
            hex_string_to_bytes(hex_str, bytes, 16);
            bytes_to_state(bytes, s);

            // chiffre le clair 
            AES_Encrypt(s, roundkeys, 4);

            // convertit le chiffré en str hex
            state_to_bytes(s, bytes);
            bytes_to_hex_string(bytes, 16, hex_str);

            // print le chiffré en str hex dans le file
            fprintf(fichier, "Results = %s\n", hex_str);
        }
    }

    fclose(fichier);
}

/* genere num_sets_5r sets (besoin que de 6 pour l'attaque sur 5 tours)  */
void gen_sets_5r(key master_key){
    FILE *fichier = fopen("sets/5r_sets", "w");
    if (fichier == NULL) {
        printf("Erreur : impossible d'ouvrir le fichier 5r_sets\n");
        return;
    }
    char hex_str[2 + (16 * 2) + 1];  // "0x" + 32 caractères + '\0'
    State s;
    uint8_t bytes[16];

    key roundkeys[11];
    KeySchedule(roundkeys, master_key);
    // fflush(stdout);

    for(int set = 0; set < num_sets_5r; set ++){
        uint8_t plaintext[16] = {0};
        for(int pair = 0; pair < 256; pair++){

            // genere le clair sous forme de hex str
            plaintext[set] = (uint8_t)pair;
            bytes_to_hex_string(plaintext, 16, hex_str);


            // print le clair dans le file
            fprintf(fichier, "Encrypting %s\n", hex_str);

            // convertit le clair en state
            hex_string_to_bytes(hex_str, bytes, 16);
            bytes_to_state(bytes, s);

            // chiffre le clair 
            AES_Encrypt(s, roundkeys, 5);

            // convertit le chiffré en str hex
            state_to_bytes(s, bytes);
            bytes_to_hex_string(bytes, 16, hex_str);

            // print le chiffré en str hex dans le file
            fprintf(fichier, "Results = %s\n", hex_str);
        }
    }

    fclose(fichier);
}

// 5b 09 f7 68 bb c2 22 e4 81 16 30 a7 fe 69 c7 65
int test_gen_sets(){
    key masterkey = {0x5b, 0x09, 0xf7, 0x68, 0xbb, 0xc2, 0x22, 0xe4,
        0x81, 0x16, 0x30, 0xa7, 0xfe, 0x69, 0xc7, 0x65};
        
        
    
    key key2;
    invKeySchedule(masterkey, 5, key2);

    print_key(key2);
    

    /*
    key masterkey = {0xb8, 0x0d, 0xcc, 0x7d, 0x9b, 0x8d, 0xd1, 0x57,
                0x2a, 0x0c, 0x0f, 0x9c, 0x33, 0xa9, 0xd9, 0x29};
    */
    
    gen_sets_4r(masterkey);

    gen_sets_5r(masterkey);
    return 1;
}
