include ../Makeparams

calc_prime_table: calc_prime_table.c
	gcc $(GCC_FLAGS) -c calc_prime_table.c

hash_performance_test: hash_performance_test.c
	cd ../collections; make hash.o
	gcc $(GCC_FLAGS) hash_performance_test.c ../collections/hash.o -o hash_performance_test