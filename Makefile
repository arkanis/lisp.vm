CFLAGS = -std=c99 -Werror -Wall -Wextra -g

# Actual programs, object files are build by implicit rules
main: main.c memory.o syntax.o eval.o lvm.o
syntax_test: syntax_test.c memory.o syntax.o

# Delete everything listed in the .gitignore file, ensures that it's properly maintained.
clean:
	rm -fr `tr '\n' ' ' < .gitignore`