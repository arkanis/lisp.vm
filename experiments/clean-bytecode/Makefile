CFLAGS = -g -Wall -std=gnu99

all: tests/common_test tests/interpreter_test
	$(foreach test,$^,$(shell $(test)))

tests/sample_test: tests/test_utils.o
tests/common_test: common.o tests/test_utils.o
tests/interpreter_test: common.o interpreter.o tests/test_utils.o tests/test_helpers.o

clean:
	rm -f *.o tests/*.o tests/*_test
