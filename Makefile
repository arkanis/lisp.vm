# We ignore the unused-parameter warning since we pass the interpreter context
# (lvm_p) to every function but don't use it (yet) in some.
CFLAGS = -std=c99 -Werror -Wall -Wextra -Wno-unused-parameter -g

# Actual programs, object files are build by implicit rules
main: main.c interpreter.o memory.o syntax.o eval.o builtins.o

tests/syntax_test: tests/syntax_test.c interpreter.o memory.o syntax.o eval.o builtins.o

# Delete everything listed in the .gitignore file, ensures that it's properly maintained.
clean:
	rm -fr `tr '\n' ' ' < .gitignore`