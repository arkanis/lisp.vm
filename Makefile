# We ignore the unused-parameter warning since we pass the interpreter context
# (lvm_p) to every function but don't use it (yet) in some.
CFLAGS = -std=c99 -Werror -Wall -Wextra -Wno-unused-parameter -g

OBJS  = interpreter.o memory.o syntax.o eval.o builtins.o
TESTS = $(patsubst %.c,%,$(wildcard tests/*_test.c))

all: main $(TESTS)
	$(foreach test,$(TESTS),$(shell $(test)))
	./main


# These are the two central header files (public and private structures and
# functions). When they change we have to rebuild everything.
$(OBJS): lvm.h internals.h


# Main program (repl) and test programs, object files are created by implicit rules
main $(TESTS): $(OBJS)


# Delete everything listed in the .gitignore file, ensures that it's properly maintained.
clean:
	rm -fr `tr '\n' ' ' < .gitignore`