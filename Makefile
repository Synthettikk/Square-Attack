CFLAGS = -O3 -march=native -flto -I includes
CC = gcc

all: build/main_4r build/main_5r test/tests

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

build/main_4r: build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o build/main_4r.o build/gen_sets.o
	$(CC) $(CFLAGS) build/main_4r.o build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o build/gen_sets.o -o build/main_4r

build/main_5r: build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o build/main_5r.o build/gen_sets.o
	$(CC) $(CFLAGS) build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o build/main_5r.o build/gen_sets.o -o build/main_5r

test/tests: build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o test/tests.o build/gen_sets.o
	$(CC) $(CFLAGS) build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o test/tests.o build/gen_sets.o -o test/tests

clean:
	rm -f build/*.o test/*.o build/main_4r build/main_5r test/tests

.PHONY: all clean
