# ============ Détection du système d'exploitation (windows nous fait chier !) ============

ifeq ($(OS),Windows_NT)
	DETECTED_OS := Windows
	RM := del /Q
	EXE := .exe
else
	DETECTED_OS := Unix
	RM := rm -f
	EXE :=
endif

# ============ Variables de compilation ============

CFLAGS = -O3 -march=native -flto -I includes
CC = gcc
NVCC = nvcc
NVFLAGS = -O3 -arch=sm_75 -I includes

# ============ Cibles ============

TARGETS = build/main_4r$(EXE) build/main_5r$(EXE) build/main_cuda$(EXE) test/tests$(EXE)

all: $(TARGETS)

# ============ Cibles C ============

build/GF8_Arithmetics.o: src/GF8_Arithmetics.c
	$(CC) $(CFLAGS) -c src/GF8_Arithmetics.c -o build/GF8_Arithmetics.o

build/helpers.o: src/helpers.c
	$(CC) $(CFLAGS) -c src/helpers.c -o build/helpers.o

build/aes.o: src/aes.c
	$(CC) $(CFLAGS) -c src/aes.c -o build/aes.o

build/load_sets.o: src/load_sets.c
	$(CC) $(CFLAGS) -c src/load_sets.c -o build/load_sets.o

build/gen_sets.o: src/gen_sets.c
	$(CC) $(CFLAGS) -c src/gen_sets.c -o build/gen_sets.o

build/main_4r.o: src/main_4r.c
	$(CC) $(CFLAGS) -c src/main_4r.c -o build/main_4r.o

build/main_5r.o: src/main_5r.c
	$(CC) $(CFLAGS) -c src/main_5r.c -o build/main_5r.o

test/tests.o: test/tests.c
	$(CC) $(CFLAGS) -c test/tests.c -o test/tests.o

build/main_4r$(EXE): build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o build/main_4r.o build/gen_sets.o
	$(CC) $(CFLAGS) build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o build/main_4r.o build/gen_sets.o -o build/main_4r$(EXE)

build/main_5r$(EXE): build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o build/main_5r.o build/gen_sets.o
	$(CC) $(CFLAGS) build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o build/main_5r.o build/gen_sets.o -o build/main_5r$(EXE)

test/tests$(EXE): build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o test/tests.o build/gen_sets.o
	$(CC) $(CFLAGS) build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o test/tests.o build/gen_sets.o -o test/tests$(EXE)

# ============ Cibles CUDA ============

build/cuda_kernel.o: src/cuda_kernel.cu
	$(NVCC) $(NVFLAGS) -c src/cuda_kernel.cu -o build/cuda_kernel.o

build/main_cuda.o: src/main_cuda.c
	$(CC) $(CFLAGS) -c src/main_cuda.c -o build/main_cuda.o

build/main_cuda$(EXE): build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o build/gen_sets.o build/main_cuda.o build/cuda_kernel.o
	$(NVCC) $(NVFLAGS) build/main_cuda.o build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o build/gen_sets.o build/cuda_kernel.o -o build/main_cuda$(EXE)

# ============ Clean ============

clean:
	$(RM) build/*.o test/*.o build/main_4r$(EXE) build/main_5r$(EXE) build/main_cuda$(EXE) test/tests$(EXE)

.PHONY: all clean


