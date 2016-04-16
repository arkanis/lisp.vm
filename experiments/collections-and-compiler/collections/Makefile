include ../Makeparams

array.o: array.h array.c
	gcc $(GCC_FLAGS) -c array.c

array_test: array_test.c array.o
	cd ..; make testing.o
	gcc $(GCC_FLAGS) array_test.c array.o ../testing.o -o array_test

hash.o: hash.h hash.c
	gcc $(GCC_FLAGS) -c hash.c

hash_test: hash_test.c hash.o
	cd ..; make testing.o
	gcc $(GCC_FLAGS) hash_test.c hash.o ../testing.o -o hash_test