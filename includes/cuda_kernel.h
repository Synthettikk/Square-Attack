#ifndef CUDA_KERNEL_H
#define CUDA_KERNEL_H

#include <stdio.h>
#include <stdbool.h>
#include "aes.h"
#include "load_sets.h"


// pour dire au compilateur nvcc (C++) de laisser cette fct nommée en C dans le .o
#ifdef __cplusplus
extern "C" {
#endif

bool attack5r_4bytes_gpu(State *h_key5, const LambdaSetCollection *collection);

#ifdef __cplusplus
}
#endif


#endif
