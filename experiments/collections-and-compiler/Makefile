include Makeparams

testing.o: testing.h testing.c
	gcc $(GCC_FLAGS) -c testing.c

clean_all: clean
	cd collections; make clean
	cd experiments; make clean