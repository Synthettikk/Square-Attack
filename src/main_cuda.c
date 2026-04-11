#include "GF8_Arithmetics.h"
#include "helpers.h"
#include "aes.h"
#include "load_sets.h"
#include "cuda_kernel.h"
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    char *set;

    // Vérif qu'un argument (chemin) a été passé    
    if (argc < 2) {        
        fprintf(stderr, "Usage: %s <chemin_des_sets>\n", argv[0]);
        printf("Chemin par défaut : sets/5_rounds_ciphertexts\n");  
        set = "sets/5_rounds_ciphertexts";
    } else{
        set = argv[1];
    }

    // Vérif que le chemin existe et est lisible    
    if (access(set, F_OK) == -1) {        
        fprintf(stderr, "Erreur: le fichier '%s' n'existe pas\n", set);        
        return 1;
    }

    // calc sur le CPU (le wrapper fait ensuite la copie vers le GPU)
    init_mult_table();

    LambdaSetCollection collection = load_sets(set);

    State key5_state; // sous forme de state

    key key5, master_key;

    // attaque 
    printf("Lancement de l'attaque...\n");

    // ================= GPU ====================
    bool found = attack5r_4bytes_gpu(&key5_state, &collection);
    // =============== FIN GPU ==================

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
}
