include ../Makeparams

scanner.o: scanner.h scanner.c
	gcc $(GCC_FLAGS) -c scanner.c

scanner_test: scanner_test.c scanner.o
	cd ..; make testing.o
	gcc $(GCC_FLAGS) scanner_test.c scanner.o ../testing.o -o scanner_test