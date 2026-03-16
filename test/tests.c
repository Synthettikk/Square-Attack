// test AES

#include "../includes/aes.h"
#include "stdio.h"

int main(void){
    if(test_aes() == 0){
        printf("AES OK \n");
    } else {
        printf("PROBLEM AES \n");
        return 1;
    }
    return 0;
}