// For open_memstream()
#define _GNU_SOURCE
#include <stdio.h>

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"

#include "../lvm.h"

void test_builtins() {
	struct{ char* in; char* out; } test_cases[] = {
		{ "(+ 1 2)",    "3" },
		{ "(- 2 1)",    "1" },
		{ "(* 2 4)",    "8" },
		{ "(/ 8 2)",    "4" },
		
		{ "(cons 1 2)",          "(1 . 2)" },
		{ "(first (cons 1 2))",  "1" },
		{ "(rest  (cons 1 2))",  "2" },
		
		{ "(define x 7)", "7" },
		{ "x",            "7" },
		{ "(define x 1)", "1" },
		// FIXME: env_put seems broken...
		//{ "x",            "1" },
		
		{ "(if true 1 2)",  "1" },
		{ "(if false 1 2)", "2" },
		{ "(if true (+ 1 2) (+ 1 7))",  "3" },
		{ "(if false (+ 1 2) (+ 1 7))", "8" },
	};
	
	
	lvm_p lvm = lvm_new();
	lvm_env_p env = lvm_base_env(lvm);
	
	char* out_stream_ptr = NULL;
	size_t out_stream_size = 0;
	lvm_atom_p result = NULL;
	
	for(size_t i = 0; i < (sizeof(test_cases) / sizeof(test_cases[0])); i++) {
		char* in = test_cases[i].in;
		char* out = test_cases[i].out;
		
		FILE* in_stream = fmemopen(in, strlen(in), "r");
			lvm_atom_p ast = lvm_read(lvm, in_stream);
		fclose(in_stream);
		
		result = lvm_eval(lvm, ast, env);
		
		FILE* out_stream = open_memstream(&out_stream_ptr, &out_stream_size);
			lvm_print(lvm, out_stream, result);
		fclose(out_stream);
		
		st_check_str(out_stream_ptr, out);
		
		free(out_stream_ptr);
		out_stream_ptr = NULL;
		out_stream_size = 0;
	}
	
	lvm_destroy(lvm);
}

int main() {
	st_run(test_builtins);
	return st_show_report();
}