#ifndef GEN_SETS
#define GEN_SETS

#include "../includes/aes.h"

/* genere num_sets_4r sets (besoin que de 3 pour l'attaque sur 4 tours)  */
void gen_sets_4r(key master_key);

/* genere num_sets_5r sets (besoin que de 6 pour l'attaque sur 5 tours)  */
void gen_sets_5r(key master_key);

/* test */
int test_gen_sets();

#endif
