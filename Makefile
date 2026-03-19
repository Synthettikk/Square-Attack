all: build/main_4r test/tests

build/GF8_Arithmetics.o: src/GF8_Arithmetics.c
	gcc -I include -c src/GF8_Arithmetics.c -o build/GF8_Arithmetics.o

build/helpers.o: src/helpers.c
	gcc -I include -c src/helpers.c -o build/helpers.o

build/aes.o: src/aes.c
	gcc -I include -c src/aes.c -o build/aes.o

build/load_sets.o: src/load_sets.c
	gcc -I include -c src/load_sets.c -o build/load_sets.o

build/main_4r.o: src/main_4r.c
	gcc -I include -c src/main_4r.c -o build/main_4r.o

test/tests.o: test/tests.c
	gcc -c test/tests.c -o test/tests.o

build/main_4r: build/main_4r.o build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o
	gcc build/main_4r.o build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o -o build/main_4r

test/tests: test/tests.o build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o
	gcc test/tests.o build/GF8_Arithmetics.o build/helpers.o build/aes.o build/load_sets.o -o test/tests

clean:
	rm -f build/*.o test/*.o build/main_4r test/tests

.PHONY: all clean

