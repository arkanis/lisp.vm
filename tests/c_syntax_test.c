// For open_memstream()
#define _GNU_SOURCE
#include <stdio.h>

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"

#include "../lvm.h"

lvm_atom_p lvm_c_read(lvm_p lvm, FILE* input);
void lvm_c_print(lvm_p lvm, FILE* output, lvm_atom_p atom);

void test_syntax() {
	struct{ char* in; char* out; } test_cases[] = {
		{ " ", NULL },
		{ "\"hello\"", "\"hello\"" },
		{ "3131", "3131" },
		{ "_sym", "_sym" },
		{ "nil", "nil" },
		{ "true", "true" },
		{ "false", "false" },
		
		{ "{}", "(begin)" },
		{ "{ a b c }", "(begin a b c)" },
		{ " { a { x y z } c } ", "(begin a (begin x y z) c)" },
		{ "{a{x y z}c}", "(begin a (begin x y z) c)" },
		
		{ "if true a", "(if true a)" },
		{ "if true a else b", "(if true a b)" },
		{ "if true {a b} else {c d}", "(if true (begin a b) (begin c d))" },
		
		{ "(a)", "a" },
		{ " ( a ) ", "a" },
		{ "()", NULL },
		{ "(a", NULL },
		
		{ "func(a, b, c) a", "(lambda (a b c) a)" },
		{ "func(a) a", "(lambda (a) a)" },
		{ "func() a", "(lambda nil a)" },
		{ "func(a, b, c) { a b }", "(lambda (a b c) (begin a b))" },
		{ " func ( ) a ", "(lambda nil a)" },
		
		{ "[]", "(quote nil)" },
		{ "[a, b, c]", "(quote (a b c))" },
		{ " [ a , b , c ] ", "(quote (a b c))" },
		
		{ "a + b", "(+ a b)" },
		{ "a - b", "(- a b)" },
		{ "a * b", "(* a b)" },
		{ "a / b", "(/ a b)" },
		{ "a < b", "(< a b)" },
		{ "a > b", "(> a b)" },
		{ "a == b", "(= a b)" },
		
		{ "(a + b)", "(+ a b)" },
		{ "(a + b) * (a + b)", "(* (+ a b) (+ a b))" },
		
		{ "a = b", "(define a b)" },
		{ "a = b + c", "(define a (+ b c))" },
		
		{ "a(b, c)", "(a b c)" },
		{ "a(b)", "(a b)" },
		{ "a()", "(a)" },
		{ " a ( b , c ) ", "(a b c)" },
		{ "a(b + 1, (c + 1) * 2)", "(a (+ b 1) (* (+ c 1) 2))" },
		
		/*
		{ "(a b c)", "(a b c)" },
		{ "(a (1 2 3) x (4 5) c)", "(a (1 2 3) x (4 5) c)" },
		
		
		{ "  3131", "3131" },
		{ "  \"hello\"", "\"hello\"" },
		{ "  _sym", "_sym" },
		{ "  ( a    b   c   ) ", "(a b c)" },
		{ "  nil", "nil" },
		{ "  true", "true" },
		{ "  false", "false" },
		{ "  (  )  ", "nil" },
		
		{ "'foo",     "(quote foo)" },
		{ "'(+ a b)", "(quote (+ a b))" },
		*/
	};
	
	char* out_stream_ptr = NULL;
	size_t out_stream_size = 0;
	lvm_p lvm = lvm_new();
	
	for(size_t i = 0; i < (sizeof(test_cases) / sizeof(test_cases[0])); i++) {
		char* in = test_cases[i].in;
		char* out = test_cases[i].out;
		
		FILE* in_stream = fmemopen(in, strlen(in), "r");
			lvm_atom_p ast = lvm_c_read(lvm, in_stream);
		fclose(in_stream);
		
		if (out == NULL) {
			st_check_null(ast);
		} else {
			FILE* out_stream = open_memstream(&out_stream_ptr, &out_stream_size);
				lvm_print(lvm, out_stream, ast);
			fclose(out_stream);
			
			st_check_str(out_stream_ptr, out);
			
			free(out_stream_ptr);
			out_stream_ptr = NULL;
			out_stream_size = 0;
		}
	}
	
	lvm_destroy(lvm);
}


int main() {
	st_run(test_syntax);
	return st_show_report();
}