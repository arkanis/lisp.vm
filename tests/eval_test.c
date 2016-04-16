// For open_memstream()
#define _GNU_SOURCE
#include <stdio.h>

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"

#include "../lvm.h"

size_t builtin_call_count = 0;
lvm_atom_p builtin_test_func(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	builtin_call_count++;
	if (argc > 0)
		return argv[0];
	return lvm_nil_atom(lvm);
}


void test_eval_rules() {
	struct{ char* in; char* out; } test_cases[] = {
		// Atoms eval to themselfs
		{ "nil",     "nil" },
		{ "true",    "true" },
		{ "false",   "false" },
		{ "3131",    "3131" },
		{ "\"str\"", "\"str\"" },
		
		// Symbol eval into env lookups
		{ "a", "17" },
		
		// Builtin should get evaled arguments (the builtin returns it's first arg)
		{ "(builtin a)", "17" }
	};
	
	
	lvm_p lvm = lvm_new();
	lvm_env_p env = lvm_env_new(lvm, NULL);
	lvm_env_put(lvm, env, "a",       lvm_num_atom(lvm, 17));
	lvm_atom_p builtin_atom = lvm_builtin_atom(lvm, builtin_test_func);
	lvm_env_put(lvm, env, "builtin", builtin_atom);
	
	char* out_stream_ptr = NULL;
	size_t out_stream_size = 0;
	lvm_atom_p result = NULL;
	
	for(size_t i = 0; i < (sizeof(test_cases) / sizeof(test_cases[0])); i++) {
		char* in = test_cases[i].in;
		char* out = test_cases[i].out;
		
		FILE* in_stream = fmemopen(in, strlen(in), "r");
			lvm_atom_p ast = lvm_read(lvm, in_stream, stderr);
		fclose(in_stream);
		
		result = lvm_eval(lvm, ast, env, stderr);
		
		FILE* out_stream = open_memstream(&out_stream_ptr, &out_stream_size);
			lvm_print(lvm, result, out_stream);
		fclose(out_stream);
		
		st_check_str(out_stream_ptr, out);
		
		free(out_stream_ptr);
		out_stream_ptr = NULL;
		out_stream_size = 0;
	}
	
	// Builtins also eval to themselfs (for now)
	result = lvm_eval(lvm, builtin_atom, env, stderr);
	st_check_msg(result == builtin_atom, "builtin doesn't eval to itself");
	
	// Lists eval to calls to builtins
	size_t old_builtin_call_count = builtin_call_count;
	lvm_atom_p ast = lvm_pair_atom(lvm, builtin_atom, lvm_pair_atom(lvm, lvm_true_atom(lvm), lvm_nil_atom(lvm)));
	result = lvm_eval(lvm, ast, env, stderr);
	st_check_int(builtin_call_count, old_builtin_call_count + 1);
	st_check_msg(result == lvm_true_atom(lvm), "builtin didn't return the right atom");
	
	lvm_env_destroy(lvm, env);
	lvm_destroy(lvm);
}

int main() {
	st_run(test_eval_rules);
	return st_show_report();
}