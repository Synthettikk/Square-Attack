// attaque sur aes 4 tours

#include "GF8_Arithmetics.h"
#include "aes.h"
#include "load_sets.h"
#include "helpers.h"
#include <time.h>
#include <unistd.h>
#include <stdio.h>


void print_votes(int votes[4][4][256], int x, int y){
    printf("Votes pour [%d][%d]:\n", x, y);
    for(int K4 = 0; K4 < 256; K4++) {
        if(votes[x][y][K4] > 0) {
            printf("K4=%02x : %d votes\n", K4, votes[x][y][K4]);
        }
    }
}


void attack4r(State key, const LambdaSetCollection *collection){
    int votes[4][4][256];
    memset(votes, 0, sizeof(votes));

    // Pour chaque cellule de la clé K4
    for(int x = 0; x < 4; x += 1){
        for(int y = 0; y < 4; y += 1){
            
            // Pour chaque hypothèse de clé (0-255)
            for(int guess = 0; guess < 256; guess += 1){
                int balanced_count = 0;
                
                // Pour chaque lambda-set
                for(int set = 0; set < collection->count; set += 1){
                    uint8_t acc = 0;
                    
                    // Pour chaque cipher de ce lambda-set
                    for(int cipher_idx = 0; cipher_idx < 256; cipher_idx += 1){
                        uint8_t byte = collection->sets[set].pairs[cipher_idx]
                                       .ciphertext_state[x][y];
                        
                        // ARK4 sur cette cellule uniquement
                        byte ^= (uint8_t)guess;
                        
                        // InvSB
                        byte = InvSubByte(byte);

                        // pas d'invSR : ce qui nous intéresse cest la position de la cellule de clé K4,
                        // pas la position au début du tour 4
                        // de toutes façons toutes les cellules en entrée du T4 doivent etre equilibrées
                        
                        // XOR dans l'accumulateur
                        acc ^= byte;
                    }
                    
                    // Si équilibré pour ce lambda-set, compte un vote
                    if(acc == 0){
                        balanced_count++;
                    }
                }
                
                // Vote = nombre de lambda-sets où cette cellule est équilibrée
                votes[x][y][guess] = balanced_count;
            }
        }
    }
    
    // Récupère le guess avec le plus de votes pour chaque cellule
    for(int x = 0; x < 4; x += 1){
        for(int y = 0; y < 4; y += 1){

            // décommenter pour afficher les votes
            // print_votes(votes, x, y);
            
            uint8_t best_guess = 0;
            int max_votes = 0;
            for(int guess = 0; guess < 256; guess += 1){
                if(votes[x][y][guess] > max_votes){
                    max_votes = votes[x][y][guess];
                    best_guess = guess;
                }
            }
            key[x][y] = best_guess;
        }
    }
}



int main(int argc, char *argv[]){

    char *set;

    // Vérif qu'un argument (chemin) a été passé    
    if (argc < 2) {        
        fprintf(stderr, "Usage: %s <chemin_des_sets>\n", argv[0]);
        printf("Chemin par défaut : sets/4_rounds_ciphertexts\n");  
        set = "sets/4_rounds_ciphertexts";
    } else{
        set = argv[1];
    }

    // Vérif que le chemin existe et est lisible    
    if (access(set, F_OK) == -1) {        
        fprintf(stderr, "Erreur: le fichier '%s' n'existe pas\n", set);        
        return 1;
    }

    init_mult_table();

    LambdaSetCollection collection = load_sets(set);
    State key4_state; // sous forme de state
    key key4;
    key master_key;

    // attaque et mesure le temps 
    printf("Lancement de l'attaque...\n");
    clock_t start, end;
    start = clock();
    // recupere la clé de tour 4 et la met dans key4_state
    attack4r(key4_state, &collection);    
    end = clock();
    printf("Clé du tour 4 trouvée : ");
    print_state(key4_state);
    printf("Durée de l'attaque : %.4f secondes \n", (double)(end-start)/CLOCKS_PER_SEC);

    state_to_bytes(key4_state, key4);

    // remonte K4 à master_key
    invKeySchedule(key4, 4, master_key);
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
    AES_Encrypt(text,round_keys, 4);

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
