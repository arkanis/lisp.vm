#include <stdbool.h>
#include <stdio.h>
#include "lvm.h"

lvm_atom_p print(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	printf("Hello from main :)\n");
	return lvm_nil_atom(lvm);
}

int main() {
	lvm_p lvm = lvm_new();
	
	lvm_env_p local_env = lvm_env_new(lvm, lvm_base_env(lvm));
	lvm_env_put(lvm, local_env, "a", lvm_str_atom(lvm, "hello"));
	lvm_env_put(lvm, local_env, "b", lvm_str_atom(lvm, "world"));
	lvm_env_put(lvm, local_env, "print", lvm_builtin_atom(lvm, print));
	
	while (true) {
		printf("> ");
		fflush(stdout);
		lvm_atom_p ast = lvm_read(lvm, stdin, stderr);
		if (ast == NULL)  // exit loop if lvm_read() gets an EOF
			break;
		lvm_atom_p result = lvm_eval(lvm, ast, local_env, stderr);
		lvm_print(lvm, result, stdout);
		printf("\n");
	}
	printf("\n");
	
	lvm_env_destroy(lvm, local_env);
	lvm_destroy(lvm);
	
	return 0;
}