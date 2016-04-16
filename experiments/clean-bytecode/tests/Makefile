GCC_ARGS = -Wall -std=gnu99 -g
LINKER_ARGS = 

%.o: %.c %.h
	gcc $(GCC_ARGS) -c $<


all: common_test interpreter_test
	./common_test
	./interpreter_test

sample_test: sample_test.c test_utils.o
	gcc $(GCC_ARGS) $< *.o -o $@

common_test: common_test.c test_utils.o
	cd ..; make common.o
	gcc $(GCC_ARGS) $< *.o ../common.o -o $@

interpreter_test: interpreter_test.c ../interpreter.h ../interpreter.c ../common.h ../common.c test_utils.o test_helpers.o
	cd ..; make interpreter.o common.o
	gcc $(GCC_ARGS) $< *.o ../interpreter.o ../common.o -o $@


clean:
	rm -f *.o *_test