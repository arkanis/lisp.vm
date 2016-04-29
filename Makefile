# We ignore the unused-parameter warning since we pass the interpreter context
# (lvm_p) to every function but don't use it (yet) in some.
CFLAGS = -std=c99 -Werror -Wall -Wextra -Wno-unused-parameter -g

all: main tests/syntax_test tests/eval_test tests/builtins_test
	./tests/syntax_test
	./tests/eval_test
	./tests/builtins_test
	./main

OBJS = interpreter.o memory.o syntax.o eval.o builtins.o

# These are the two central header files (public and private structures and
# functions). When they change we have to rebuild everything.
$(OBJS): lvm.h common.h


# Actual programs, object files are build by implicit rules
main: main.c $(OBJS)

# Tests
tests/syntax_test: tests/syntax_test.c $(OBJS)
tests/eval_test: tests/eval_test.c $(OBJS)
tests/builtins_test: tests/builtins_test.c $(OBJS)


# Delete everything listed in the .gitignore file, ensures that it's properly maintained.
clean:
	rm -fr `tr '\n' ' ' < .gitignore`