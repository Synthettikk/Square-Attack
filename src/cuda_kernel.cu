/*
Réécriture de l'attaque square sur aes 5 tours en CUDA pour paralléliser

Host noté h est le processeur CPU
Device noté d est la carte graphique GPU

Les vars __constant__ vont dans la mémoire rapide (max 64Ko), les vars __device__ vont dans la mémmoire globale du gpu

Architecture GPU : 
GPU
├── SM (Streaming Multiprocessor) — ~100-200 par GPU
│   ├── Warp (32 threads) — exécutés en lockstep
│   ├── Warp
│   ├── Warp
│   └── ...
├── SM
└── ...

Architecture logicielle CUDA :
Grid (tout ce qu'on lance)
├── Block (groupe de threads, s'exécute sur 1 SM)
│   ├── Thread 0
│   ├── Thread 1
│   ├── ...
│   └── Thread 255
├── Block
├── Block
└── ...
Pour faire simple : on a au mieux 1 block pour 1 SM
Dans CUDA c'est nous qui choisisons le nombre de blocks et le nombre de threads/block que l'on veut lancer, ce qui nous 
donne un nombre de threads total.

Le but c'est d'occuper le GPU au max pour qu'un max de SM/threads travaillent en parallèle. 
On a la formule : Occupancy = (Blocks en vol / Blocks totaux) × 100%

Faisons le calcul avec les GPU à disposition : RTX 2060 (30 SM / 1920 threads) et RTX 2080 Super (48 SM / 3072 threads)

// 256^2 = 65,536 threads total
// blocks = 65,536 / 1024 = 64 blocs

// RTX 2060 : 30 SMs × 2 blocs/SM = 60 blocs actifs
// Occupancy = 60 / 64 = 93.75% 

*/


#include "cuda_kernel.h"
#include <cuda_runtime.h> // allocation mémoire, copies, lancements de kernels (cudaMalloc, cudaMemcpy, etc.)
#include <device_launch_parameters.h> // définit blockIdx, threadIdx, gridDim, blockDim 
#include <stdio.h>
#include <stdbool.h>
#include <chrono>
#include <thread>
#include "GF8_Arithmetics.h"
#include "aes.h"
#include "load_sets.h"

// check si l'appel cuda a marché
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = (call); \
        if (err != cudaSuccess) { \
            printf("CUDA error: %s\n", cudaGetErrorString(err)); \
            return false; \
        } \
    } while(0) // while(0) s'arrete apres 1 tour
    // ce do while sert juste à structurer en bloc (evite la casse si appelé dans un if sans accolades)


typedef struct { uint8_t c; uint8_t r; } Pos;


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
__constant__ Pos d_positions[4][4] = {
    // pour x = 0 : [0][0], [3][1], [2][2], [1][3]
    { {0,0}, {3,1}, {2,2}, {1,3} },

    // pour x = 1 : [1][0], [0][1], [3][2], [2][3]
    { {1,0}, {0,1}, {3,2}, {2,3} },

    // pour x = 2 : [2][0], [1][1], [0][2], {3,3}
    { {2,0}, {1,1}, {0,2}, {3,3} },

    // pour x = 3 : [3][0], [2][1], [1][2], [0][3}
    { {3,0}, {2,1}, {1,2}, {0,3} }
};

// inv sbox precalc gpu
__constant__ uint8_t d_inv_sbox[256] = {
  // 256 values...
  0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
  0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
  0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
  0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
  0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
  0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
  0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
  0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
  0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
  0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
  0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
  0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
  0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
  0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
  0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
  0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

// ============== Reecriture des fcts AES nécessaires pour le device ===================

// version gpu (trop gros pour etre passé en constant)
__device__ uint8_t d_mult_table[256][256];

// le precalcul de la mult_table reste CPU

__device__ static inline uint8_t d_mult(uint8_t a, uint8_t b){
    return d_mult_table[a][b];
}

__device__ static inline uint8_t d_InvSubByte(uint8_t byte){
    return d_inv_sbox[byte];
}

__device__ static inline void d_InvMixColumn(uint8_t col[4]) {
    uint8_t a = col[0], b = col[1], c = col[2], d = col[3];
    col[0] = d_mult(0x0e, a) ^ d_mult(0x0b, b) ^ d_mult(0x0d, c) ^ d_mult(0x09, d);
    col[1] = d_mult(0x09, a) ^ d_mult(0x0e, b) ^ d_mult(0x0b, c) ^ d_mult(0x0d, d);
    col[2] = d_mult(0x0d, a) ^ d_mult(0x09, b) ^ d_mult(0x0e, c) ^ d_mult(0x0b, d);
    col[3] = d_mult(0x0b, a) ^ d_mult(0x0d, b) ^ d_mult(0x09, c) ^ d_mult(0x0e, d);
}


// inverse les tours 4 et 5 d'aes 5 tours (que la col nécessaire) et return l'indice 0 de la col
__device__ static inline uint8_t d_invert_rounds(
    uint8_t col_state[4],
    uint8_t guess4,
    int col,
    const State key5
) {
    // Inv ARK5
    col_state[0] ^= key5[d_positions[col][0].c][d_positions[col][0].r];
    col_state[1] ^= key5[d_positions[col][1].c][d_positions[col][1].r];
    col_state[2] ^= key5[d_positions[col][2].c][d_positions[col][2].r];
    col_state[3] ^= key5[d_positions[col][3].c][d_positions[col][3].r];
    
    // invSB_5
    col_state[0] = d_InvSubByte(col_state[0]);
    col_state[1] = d_InvSubByte(col_state[1]);
    col_state[2] = d_InvSubByte(col_state[2]);
    col_state[3] = d_InvSubByte(col_state[3]);
    
    // invMC
    d_InvMixColumn(col_state);
    
    // Inv ARK4
    col_state[0] ^= guess4;
    
    // invSB_4
    col_state[0] = d_InvSubByte(col_state[0]);
    
    return col_state[0];
}

// pour accumuler sur un lambda-set : return true si acc == 0
__device__ bool d_check_lambda_set(
    uint8_t guess4,
    const LambdaSet *set,
    int col,
    const State key5
) {
    uint8_t acc = 0;
    
    for(int cipher_idx = 0; cipher_idx < 256; cipher_idx += 1) {
        uint8_t col_state[4];
        // copie dans col state les 4 octets ciblés
        col_state[0] = set->pairs[cipher_idx].ciphertext_state[d_positions[col][0].c][d_positions[col][0].r];
        col_state[1] = set->pairs[cipher_idx].ciphertext_state[d_positions[col][1].c][d_positions[col][1].r];
        col_state[2] = set->pairs[cipher_idx].ciphertext_state[d_positions[col][2].c][d_positions[col][2].r];
        col_state[3] = set->pairs[cipher_idx].ciphertext_state[d_positions[col][3].c][d_positions[col][3].r];
        
        acc ^= d_invert_rounds(col_state, guess4, col, key5);
    }
    
    return acc == 0;  // équilibré si acc == 0
}




__global__ void attack5r_4bytes_kernel(
    State *key5_template,
    int col,
    const LambdaSetCollection *collection,
    unsigned int *found
) {
    // Calcul thread_id en grille 2D
    uint64_t block_id = (uint64_t)blockIdx.y * gridDim.x + blockIdx.x;
    uint64_t thread_id = block_id * blockDim.x + threadIdx.x;
    
    if (thread_id >= (256ULL * 256 * 256 * 256)) return; // 256^4
    if (atomicOr(found, 0) != 0) return;
    
    // Décoder thread_id
    int guess5_byte1 = (thread_id >> 24) & 0xFF;
    int guess5_byte2 = (thread_id >> 16) & 0xFF;
    int guess5_byte3 = (thread_id >> 8) & 0xFF;
    int guess5_byte4 = thread_id & 0xFF;
    
    // Copie locale de la clé
    State local_key5;
    memcpy(local_key5, key5_template, sizeof(State));
    
    // Remplir les 4 bytes
    local_key5[d_positions[col][0].c][d_positions[col][0].r] = guess5_byte1;
    local_key5[d_positions[col][1].c][d_positions[col][1].r] = guess5_byte2;
    local_key5[d_positions[col][2].c][d_positions[col][2].r] = guess5_byte3;
    local_key5[d_positions[col][3].c][d_positions[col][3].r] = guess5_byte4;
    
    // Boucle interne : essayer tous les guess4 (256 itérations)
    for(int guess4 = 0; guess4 < 256; guess4++) {
        bool is_key = true;
        
        // Vérifier les lambda-sets
        for(int set = 0; set < collection->count; set++) {
            if (__builtin_expect(!d_check_lambda_set(guess4, &collection->sets[set], col, local_key5), 1)) {
                // printf("is_key false, set %d \n", set); (ok)
                is_key = false;
                break;
            }
        }
        
        if (is_key) {
            printf("FOUND! Col %d\n", col);
            // Copie (seulement, pour pas perturber les autres colonnes qui se font en parallèle) les 4 octets trouvés pour cette colonne
            (*key5_template)[d_positions[col][0].c][d_positions[col][0].r] = local_key5[d_positions[col][0].c][d_positions[col][0].r];
            (*key5_template)[d_positions[col][1].c][d_positions[col][1].r] = local_key5[d_positions[col][1].c][d_positions[col][1].r];
            (*key5_template)[d_positions[col][2].c][d_positions[col][2].r] = local_key5[d_positions[col][2].c][d_positions[col][2].r];
            (*key5_template)[d_positions[col][3].c][d_positions[col][3].r] = local_key5[d_positions[col][3].c][d_positions[col][3].r];
            atomicOr(found, 1);
            return;
        }
    }
}


__global__ void attack5r_2bytes_kernel(
    State *key5_template,
    int col,
    const LambdaSetCollection *collection,
    unsigned int *d_found,  // c un tableau de 4
    unsigned long long *d_progress // idem
) { 

    // Parallélise sur 2 bytes (256^2 = 65,536 threads)
    uint32_t thread_id = (uint32_t)blockIdx.x * blockDim.x + threadIdx.x;
    
    if (thread_id >= (256ULL * 256)) return;
    if (atomicOr(&d_found[col], 0) != 0) return;  // Vérifie le flag DE CETTE COLONNE
    
    // Décode les 2 bytes parallélisés (ok parcourt tous les couples)
    int guess5_byte1 = (thread_id >> 8) & 0xFF;
    int guess5_byte2 = thread_id & 0xFF;
    
    State local_key5;
    memcpy(local_key5, key5_template, sizeof(State));
    
    // modifie local_key5 avec les guess
    local_key5[d_positions[col][0].c][d_positions[col][0].r] = guess5_byte1;
    local_key5[d_positions[col][1].c][d_positions[col][1].r] = guess5_byte2;

    unsigned long long local_count = 0;  // Compteur local du nb d'itérations effectuées (pour connaitre la progression)
    
    // Double boucle pour les 2 autres bytes (256 * 256 = 65,536 itérations par thread)
    for(int guess5_byte3 = 0; guess5_byte3 < 256; guess5_byte3++) {

        
        for(int guess5_byte4 = 0; guess5_byte4 < 256; guess5_byte4++) {
            
            if (__builtin_expect(atomicOr(&d_found[col], 0) != 0, 0)) return;  // ← Check régulier pour sortir des kernels paralleles
            
            local_key5[d_positions[col][2].c][d_positions[col][2].r] = guess5_byte3;
            local_key5[d_positions[col][3].c][d_positions[col][3].r] = guess5_byte4;

            
            for(int guess4 = 0; guess4 < 256; guess4++){
                
                bool is_key = true;

                // Vérif les lambda-sets
                for(int set = 0; set < collection->count; set++) {
                    if (__builtin_expect(!d_check_lambda_set(guess4, &collection->sets[set], col, local_key5), 1)) {
                        is_key = false;
                        break;
                    }
                }

                // si on a trouvé la clé (les bons octets de la col) alors on les copie dans key_template
                if (is_key) { // on s'attend à ce que is_key soit faux
                    //printf("FOUND! Col %d\n", col);
                    // Copie (seulement, pour pas perturber les autres colonnes qui se font en parallèle) les 4 octets trouvés pour cette colonne
                    (*key5_template)[d_positions[col][0].c][d_positions[col][0].r] = local_key5[d_positions[col][0].c][d_positions[col][0].r];
                    (*key5_template)[d_positions[col][1].c][d_positions[col][1].r] = local_key5[d_positions[col][1].c][d_positions[col][1].r];
                    (*key5_template)[d_positions[col][2].c][d_positions[col][2].r] = local_key5[d_positions[col][2].c][d_positions[col][2].r];
                    (*key5_template)[d_positions[col][3].c][d_positions[col][3].r] = local_key5[d_positions[col][3].c][d_positions[col][3].r];
                    
                    atomicExch(&d_progress[col], 256ULL * 256 * 256 * 256); // qd on a trouvé la clé, met le progress au max
                    atomicOr(&d_found[col], 1);  // Set le flag POUR CETTE COLONNE
                    return;
                }
            }
            local_count++; 
        }
        atomicAdd(&d_progress[col], local_count); // toutes les 256 itérations
        local_count = 0;
    }
    // Si pas trouvé, ajoute le compteur à la fin aussi 
    atomicAdd(&d_progress[col], local_count);
}




// Wrapper CPU
bool attack5r_4bytes_gpu(State *h_key5, const LambdaSetCollection *collection) {

    // mult_table est déjà précalculée en CPU et la mémoire est déjà allouée avec device mult_table, reste à copier  
    CUDA_CHECK(cudaMemcpyToSymbol(d_mult_table,            // destination (GPU)
                                  mult_table,              // Source CPU
                                  sizeof(mult_table),      // taille
                                  0, 
                                  cudaMemcpyHostToDevice));    
    
    // def des vars que l'on veut passer au gpu

    State *d_key5;
    LambdaSetCollection *d_collection;
    LambdaSet *d_sets;
    unsigned int *d_found; // bool en int (maintenant un tableau de 4)
    unsigned int h_found[4] = {0, 0, 0, 0};

    // allocation mémoire sur gpu pour nos vars

    CUDA_CHECK(cudaMalloc(&d_key5, sizeof(State))); // malloc key5 sur gpu
    CUDA_CHECK(cudaMalloc(&d_collection, sizeof(LambdaSetCollection))); // malloc collection sur gpu
    CUDA_CHECK(cudaMalloc(&d_sets, sizeof(LambdaSet) * collection->count)); // malloc les sets sur gpu
    CUDA_CHECK(cudaMalloc(&d_found, sizeof(unsigned int) * 4)); // malloc le tableau de 4 bools found

    // copie les vars vers le gpu

    CUDA_CHECK(cudaMemcpy(d_key5, h_key5, sizeof(State), cudaMemcpyHostToDevice)); // Copie la clé vers le gpu
    CUDA_CHECK(cudaMemcpy(d_sets, collection->sets, sizeof(LambdaSet) * collection->count, cudaMemcpyHostToDevice)); // Copie les sets d'abord
    // Crée une copie de la collection avec pointeur corrigé pour gpu (marche car les pairs sont des tableaux statiques pas des pointeurs : pas besoin de copier les pairs à part)
    LambdaSetCollection h_collection_copy = *collection;
    h_collection_copy.sets = d_sets;
    CUDA_CHECK(cudaMemcpy(d_collection, &h_collection_copy, sizeof(LambdaSetCollection), cudaMemcpyHostToDevice)); // Copie la collection de sets
    CUDA_CHECK(cudaMemcpy(d_found, h_found, sizeof(unsigned int) * 4, cudaMemcpyHostToDevice));

    // ===== POUR PROGRESSION =====
    unsigned long long *d_progress;
    unsigned long long h_progress[4] = {0, 0, 0, 0};

    CUDA_CHECK(cudaMalloc(&d_progress, 4 * sizeof(unsigned long long)));
    CUDA_CHECK(cudaMemcpy(d_progress, h_progress, 4 * sizeof(unsigned long long), cudaMemcpyHostToDevice));

    // ==========================================
    
    // Lancement du  kernel

    uint32_t total_threads = 256ULL * 256;
    int threads_per_block = 1024;
    int blocks = (total_threads + threads_per_block - 1) / threads_per_block;
    
    // Parallélise les 4 colonnes

    cudaStream_t streams[4];
    cudaStream_t copy_stream; // pour la progression

    auto start_time = std::chrono::high_resolution_clock::now();

    // ya pas de race condition sur les colonnes car chaque attaque écrit seulement sur SA colonne
    for(int col = 0; col < 4; col++) {
        CUDA_CHECK(cudaStreamCreate(&streams[col]));
        CUDA_CHECK(cudaStreamCreate(&copy_stream));
        attack5r_2bytes_kernel<<<blocks, threads_per_block, 0, streams[col]>>>(
            d_key5, col, d_collection, d_found, d_progress
        );
    }

    /*
    uint64_t total_threads = 256ULL * 256 * 256 * 256; // 256^4
    int threads_per_block = 1024;
    uint32_t blocks_1d = (total_threads + threads_per_block - 1) / threads_per_block;
    
    // Grille 2D pour gérer les 4.3 milliards de threads
    dim3 grid(65535, (blocks_1d + 65535) / 65535);
    
    for(int col = 0; col < 4; col++) {
        attack5r_4bytes_kernel<<<grid, threads_per_block>>>(
            d_key5, col, d_collection, d_found
        );
    }
    */
    

    // ===== AFFICHAGE PROGRESSION =====
    
    unsigned int h_found_flags[4] = {0, 0, 0, 0};
    bool all_found = false;
    
    while(!all_found) {
        for(int col = 0; col < 4; col++) {
            CUDA_CHECK(cudaMemcpyAsync(&h_progress[col], &d_progress[col], sizeof(unsigned long long), 
                                  cudaMemcpyDeviceToHost, copy_stream)); // copy_stream
            CUDA_CHECK(cudaMemcpyAsync(&h_found_flags[col], &d_found[col], sizeof(unsigned int), 
                                  cudaMemcpyDeviceToHost, copy_stream));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // ← Attend les transfers (vraiment pas critique, au pire on affiche l'état N-1 -> décalage de 500ms)
        
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - start_time).count();
        int elapsed_int = (int)elapsed;
        int hours = elapsed_int / 3600;
        int minutes = (elapsed_int % 3600) / 60;
        int seconds = elapsed_int % 60;
        
        unsigned long long total_progress = h_progress[0] + h_progress[1] + h_progress[2] + h_progress[3];
        unsigned long long total = 256ULL * 256 * 256 * 256 * 4;
        
        double percent = (100.0 * total_progress) / total;
        //double rate = total_progress / elapsed;
        //double remaining = (rate > 0) ? (total - total_progress) / rate : 0; // pour eviter division par 0
        
        printf("\r");
        for(int col = 0; col < 4; col++) {
            double col_percent = (100.0 * h_progress[col]) / (256ULL * 256 * 256 * 256);
            
            // Affiche si la colonne a trouvé ou son pourcentage
            if (h_found_flags[col]) {
                printf("Col%d: ✓ FOUND | ", col);
                //printf("Col%d: %5.1f%% | ", col, col_percent);
            } else {
                printf("Col%d: %5.1f%% | ", col, col_percent);
            }
        }
        printf("Total: %.1f%% | Temps écoulé : %02dh %02dmin %02dsec", percent, hours, minutes, seconds);
        // on laisse pas l'ETA car c'est casse-gueule (ça pète quand ya early exit, on met le temps écoulé à la place)
        fflush(stdout);
        
        // Vérifie que TOUTES les colonnes ont trouvé
        all_found = (h_found_flags[0] && h_found_flags[1] && h_found_flags[2] && h_found_flags[3]);
        
        // attention, on sort de la boucle infinie que si on trouve toute la clé
        if (all_found) {
            printf("\nTerminaison des kernels...\n");
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    
    // ================================================

    // attend que ca se termine
    for(int i = 0; i < 4; i++) {
        CUDA_CHECK(cudaStreamSynchronize(streams[i]));
        CUDA_CHECK(cudaStreamDestroy(streams[i]));
    }
    CUDA_CHECK(cudaStreamDestroy(copy_stream));

    CUDA_CHECK(cudaGetLastError());  // Vérif erreur de lancement (pb grille, mémoire insuffisante, ...)
    CUDA_CHECK(cudaDeviceSynchronize()); // attends la fin d'exécution et verif si erreur pdt l'exec

    // printf("\nGPU DONE\n");
    
    // récup le résultat et la clé sur le cpu
    CUDA_CHECK(cudaMemcpy(&h_found, d_found, sizeof(unsigned int) * 4, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_key5, d_key5, sizeof(State), cudaMemcpyDeviceToHost));
    
    // Libération mémoire
    CUDA_CHECK(cudaFree(d_key5));
    CUDA_CHECK(cudaFree(d_collection));
    CUDA_CHECK(cudaFree(d_sets));
    CUDA_CHECK(cudaFree(d_found));

    // ===== Libére d_progress =====
    CUDA_CHECK(cudaFree(d_progress));
    // ========================================

    return (h_found[0] && h_found[1] && h_found[2] && h_found[3]);  // verif que tout est trouvé
}
