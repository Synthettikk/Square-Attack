// test AES

#include "../includes/aes.h"
#include "../includes/load_sets.h"
#include "../includes/gen_sets.h"
#include "stdio.h"

int main(void){
    if(test_aes() == 0){
        printf("AES OK \n");
    } else {
        printf("PROBLEM AES \n");
        return 1;
    }

    if(test_load_sets()){
        printf("LOAD SETS OK \n");
    } else {
        printf("PROBLEM LOAD SETS");
    }

    if(test_gen_sets()){
        printf("GEN SETS OK \n");
    } else {
        printf("PROBLEM GEN SETS");
    }
    return 0;
}