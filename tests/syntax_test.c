// For open_memstream()
#define _GNU_SOURCE
#include <stdio.h>

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"

#include "../lvm.h"


void test_syntax() {
	struct{ char* in; char* out; } test_cases[] = {
		{ "3131", "3131" },
		{ "\"hello\"", "\"hello\"" },
		{ "_sym", "_sym" },
		{ "(a b c)", "(a b c)" },
		{ "(a (1 2 3) x (4 5) c)", "(a (1 2 3) x (4 5) c)" },
		{ "nil", "nil" },
		{ "true", "true" },
		{ "false", "false" },
		{ " ", NULL },
		
		{ "  3131", "3131" },
		{ "  \"hello\"", "\"hello\"" },
		{ "  _sym", "_sym" },
		{ "  ( a    b   c   ) ", "(a b c)" },
		{ "  nil", "nil" },
		{ "  true", "true" },
		{ "  false", "false" },
		{ "  (  )  ", "nil" }
	};
	
	char* out_stream_ptr = NULL;
	size_t out_stream_size = 0;
	lvm_p lvm = lvm_new();
	
	for(size_t i = 0; i < (sizeof(test_cases) / sizeof(test_cases[0])); i++) {
		char* in = test_cases[i].in;
		char* out = test_cases[i].out;
		
		FILE* in_stream = fmemopen(in, strlen(in), "r");
			lvm_atom_p ast = lvm_read(lvm, in_stream, stderr);
		fclose(in_stream);
		
		if (out == NULL) {
			st_check_null(ast);
		} else {
			FILE* out_stream = open_memstream(&out_stream_ptr, &out_stream_size);
				lvm_print(lvm, ast, out_stream);
			fclose(out_stream);
			
			st_check_str(out_stream_ptr, out);
			
			free(out_stream_ptr);
			out_stream_ptr = NULL;
			out_stream_size = 0;
		}
	}
	
	lvm_destroy(lvm);
}

void test_symbol_pooling() {
	lvm_p lvm = lvm_new();
	
	FILE* in_stream = NULL;
	char* symbol_str = "some_fancy_symbol_name";
	
	in_stream = fmemopen(symbol_str, strlen(symbol_str), "r");
		lvm_atom_p symbol_atom_1 = lvm_read(lvm, in_stream, stderr);
	fclose(in_stream);
	
	in_stream = fmemopen(symbol_str, strlen(symbol_str), "r");
		lvm_atom_p symbol_atom_2 = lvm_read(lvm, in_stream, stderr);
	fclose(in_stream);
	
	in_stream = fmemopen(symbol_str, strlen(symbol_str), "r");
		lvm_atom_p symbol_atom_3 = lvm_read(lvm, in_stream, stderr);
	fclose(in_stream);
	
	st_check(symbol_atom_1 == symbol_atom_2);
	st_check(symbol_atom_2 == symbol_atom_3);
	
	lvm_destroy(lvm);
}


int main() {
	st_run(test_syntax);
	st_run(test_symbol_pooling);
	return st_show_report();
}