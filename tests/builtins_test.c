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
		
		{ "(quote 1)",        "1" },
		{ "(quote sym)",      "sym" },
		{ "(quote \"foo\")",  "\"foo\"" },
		{ "(quote (+ 1 2))",  "(+ 1 2)" },
		
		{ "(= 1 1)", "true" },
		{ "(= 2 1)", "false" },
		{ "(= true true)",   "true" },
		{ "(= false true)",  "false" },
		{ "(= false false)", "true" },
		{ "(= \"foo\" \"foo\")", "true" },
		{ "(= \"foo\" \"bar\")", "false" },
		
		{ "(= (quote (1 2)) (quote (1 2)))",         "true" },
		{ "(= (quote (1 2)) (quote (1 1)))",         "false" },
		{ "(= (quote (1 2)) (quote 1))",             "false" },
		{ "(= (quote (1 (2 3))) (quote (1 (2 1))))", "false" },
		
		{ "(< 1 2)", "true" },
		{ "(< 2 1)", "false" },
		{ "(< 1 1)", "false" },
		{ "(> 1 2)", "false" },
		{ "(> 2 1)", "true" },
		{ "(> 1 1)", "false" },
		
		{ "(define x 7)", "7" },
		{ "x",            "7" },
		{ "(define x 1)", "1" },
		{ "x",            "1" },
		
		// Make sure error atoms are not assigned as values
		{ "(define x y)", NULL },
		{ "x",            "1" },
		
		{ "(if true 1 2)",  "1" },
		{ "(if false 1 2)", "2" },
		{ "(if true (+ 1 2) (+ 1 7))",  "3" },
		{ "(if false (+ 1 2) (+ 1 7))", "8" },
		
		// Basic lambda definition and calling
		{ "(lambda (a b) (+ a b))", "(lambda (a b) (+ a b))" },
		{ "(define plus (lambda (a b) (+ a b)))", "(lambda (a b) (+ a b))" },
		{ "(plus 1 2)", "3" },
		
		// Stop arg eval and call as soon as an error atom is encountered
		{ "(plus unknown 2)", NULL },
		{ "(plus (+ (+ (+ unknown 1) 1) 1) 2)", NULL },
		
		// Lambdas with multiple expressions in body
		{ "(lambda (a b) (+ a 1) (+ b 1) (+ a b))", "(lambda (a b) (+ a 1) (+ b 1) (+ a b))" },
		{ "(define plus (lambda (a b) (+ a 1) (+ b 1) (+ a b)))", "(lambda (a b) (+ a 1) (+ b 1) (+ a b))" },
		{ "(plus 1 2)", "3" },
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
		
		if (out) {
			// We expect a proper result atom
			FILE* out_stream = open_memstream(&out_stream_ptr, &out_stream_size);
				lvm_print(lvm, out_stream, result);
			fclose(out_stream);
			
			st_check_str(out_stream_ptr, out);
			
			free(out_stream_ptr);
			out_stream_ptr = NULL;
			out_stream_size = 0;
		} else {
			// We expect an error atom with an error message
			st_check_int(result->type, LVM_T_ERROR);
			st_check_not_null(result->str);
		}
	}
	
	lvm_destroy(lvm);
}

int main() {
	st_run(test_builtins);
	return st_show_report();
}