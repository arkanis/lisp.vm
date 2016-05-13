#include <string.h>
#include <ctype.h>
#include "internals.h"

/** C style syntax

Simple literals:

3141 → LVM_T_NUM
"foo bar" → LVM_T_STR
nil → LVM_T_NIL
true → LVM_T_TRUE
false → LVM_T_FALSE
foo_bar → LVM_T_SYM

More complex:

( expr ) → expr
{ expr expr ... } → (begin expr expr ...)
a(b, c) → (a b c) → (a b c)
foo = 123 → (define foo 123)
a + b → (+ a b)   a op b, with op = + - * / == < >

if expr expr else expr → (if expr expr expr)
func(arg1, arg2, ...) { body ... } → (lambda (arg1 arg2 ...) body ...)

[a, b, c] → (quote (a b c))

TODO:

:sym → (quote sym)

**/

static int lvm_next_char_after_whitespaces(FILE* input);
static lvm_atom_p lvm_c_read_expr(lvm_p lvm, FILE* input);

lvm_atom_p lvm_c_read(lvm_p lvm, FILE* input) {
	lvm_atom_p expr = lvm_c_read_expr(lvm, input);
	
	int c = lvm_next_char_after_whitespaces(input);
	switch(c) {
		case '(':
			{
				lvm_atom_p args = lvm_pair_atom(lvm,
					expr,
					lvm_nil_atom(lvm)
				);
				lvm_atom_p current_arg = args;
				while (true) {
					c = lvm_next_char_after_whitespaces(input);
					if (c == ')')  // End of arg list
						break;
					else
						ungetc(c, input);
					
					current_arg->rest = lvm_pair_atom(lvm, lvm_c_read(lvm, input), current_arg->rest);
					current_arg = current_arg->rest;
					
					c = lvm_next_char_after_whitespaces(input);
					if (c == ',') {  // Next arg
						// Continue with next iteration
					} else if (c == ')') {
						break;
					} else {
						fprintf(stderr, "']' or ',' expected after list element\n");
						return NULL;
					}
				}
				
				return args;
			}
			break;
		case '=':
			if ( (c = getc(input)) == '=' ) {
				// got "==", this maps to the "=" builtin
				return lvm_pair_atom(lvm,
					lvm_sym_atom(lvm, "="),
					lvm_pair_atom(lvm,
						expr,
						lvm_pair_atom(lvm,
							lvm_c_read(lvm, input),
							lvm_nil_atom(lvm)
						)
					)
				);
			} else {
				// got "=", this maps to the define builtin
				// foo = 123 → (define foo 123)
				ungetc(c, input);
				
				lvm_atom_p value = lvm_c_read(lvm, input);
				return lvm_pair_atom(lvm,
					lvm_sym_atom(lvm, "define"),
					lvm_pair_atom(lvm,
						expr,
						lvm_pair_atom(lvm,
							value,
							lvm_nil_atom(lvm)
						)
					)
				);
			}
		case '+':
		case '-':
		case '*':
		case '/':
		case '<':
		case '>':
			{
				char* op_name = malloc(2);
				op_name[0] = c;
				op_name[1] = '\0';
				return lvm_pair_atom(lvm,
					lvm_sym_atom(lvm, op_name),
					lvm_pair_atom(lvm,
						expr,
						lvm_pair_atom(lvm,
							lvm_c_read(lvm, input),
							lvm_nil_atom(lvm)
						)
					)
				);
			}
		case ';':
			// Manual way to end an expression
			break;
		default:
			ungetc(c, input);
	}
	
	return expr;
}

static lvm_atom_p lvm_c_read_expr(lvm_p lvm, FILE* input) {
	int c = lvm_next_char_after_whitespaces(input);
	char* str = NULL;
	switch(c) {
		case EOF:
			return NULL;
		case '"':
			if ( fscanf(input, "%m[^\"]\"", &str) != 1 )
				return NULL;
			return lvm_str_atom(lvm, str);
		case '{':
			{
				lvm_atom_p ast = lvm_pair_atom(lvm, lvm_sym_atom(lvm, "begin"), lvm_nil_atom(lvm));
				lvm_atom_p current_pair = ast;
				while ( (c = lvm_next_char_after_whitespaces(input)) != '}' ) {
					ungetc(c, input);
					current_pair->rest = lvm_pair_atom(lvm, lvm_c_read(lvm, input), current_pair->rest);
					current_pair = current_pair->rest;
				}
				return ast;
			}
		case '(':
			{
				lvm_atom_p expr = lvm_c_read(lvm, input);
				if ( (c = lvm_next_char_after_whitespaces(input)) != ')' ) {
					fprintf(stderr, "Missing ')' after expr\n");
					return NULL;
				}
				return expr;
			}
		case '[':
			{
				lvm_atom_p args = lvm_pair_atom(lvm,
					lvm_nil_atom(lvm),
					lvm_nil_atom(lvm)
				);
				lvm_atom_p current_arg = args;
				while (true) {
					c = lvm_next_char_after_whitespaces(input);
					if (c == ']')  // End of arg list
						break;
					else
						ungetc(c, input);
					
					current_arg->rest = lvm_pair_atom(lvm, lvm_c_read(lvm, input), current_arg->rest);
					current_arg = current_arg->rest;
					
					c = lvm_next_char_after_whitespaces(input);
					if (c == ',') {  // Next arg
						// Continue with next iteration
					} else if (c == ']') {
						break;
					} else {
						fprintf(stderr, "']' or ',' expected after list element\n");
						return NULL;
					}
				}
				// Strip the first unnecessary nil, we just put it there so the loop doesn't
				// need a special case to append the first arg.
				args = args->rest;
				
				return lvm_pair_atom(lvm,
					lvm_sym_atom(lvm, "quote"),
					lvm_pair_atom(lvm,
						args,
						lvm_nil_atom(lvm)
					)
				);
			}
		default:
			ungetc(c, input);
			break;
	}
	
	if (c >= '0' && c <= '9') {
		int64_t num = 0;
		if ( fscanf(input, "%lu", (uint64_t*)&num) != 1 )
			return NULL;
		return lvm_num_atom(lvm, num);
	}
	
	// We got either a keyword or a symbol
	if ( fscanf(input, " %m[_a-zA-Z0-9]", &str) != 1 )
		return NULL;
	
	if (strcmp(str, "nil") == 0)
		return lvm_nil_atom(lvm);
	else if (strcmp(str, "true") == 0)
		return lvm_true_atom(lvm);
	else if (strcmp(str, "false") == 0)
		return lvm_false_atom(lvm);
	else if (strcmp(str, "if") == 0) {
		lvm_atom_p cond_expr = lvm_c_read(lvm, input);
		lvm_atom_p true_case = lvm_c_read(lvm, input);
		
		int c1, c2, c3, c4;
		if ( (c1 = lvm_next_char_after_whitespaces(input)) == 'e' ) {
			if ( (c2 = lvm_next_char_after_whitespaces(input)) == 'l' ) {
				if ( (c3 = lvm_next_char_after_whitespaces(input)) == 's' ) {
					if ( (c4 = lvm_next_char_after_whitespaces(input)) == 'e' ) {
						// Got a complete "else" after the true case
						lvm_atom_p false_case = lvm_c_read(lvm, input);
						return lvm_pair_atom(lvm,
							lvm_sym_atom(lvm, "if"),
							lvm_pair_atom(lvm,
								cond_expr,
								lvm_pair_atom(lvm,
									true_case,
									lvm_pair_atom(lvm,
										false_case,
										lvm_nil_atom(lvm)
									)
								)
							)
						);
					} else {
						ungetc(c4, input);
					}
				} else {
					ungetc(c3, input);
				}
			} else {
				ungetc(c2, input);
			}
		} else {
			ungetc(c1, input);
		}
		
		// Didn't get an "else"
		return lvm_pair_atom(lvm,
			lvm_sym_atom(lvm, "if"),
			lvm_pair_atom(lvm,
				cond_expr,
				lvm_pair_atom(lvm,
					true_case,
					lvm_nil_atom(lvm)
				)
			)
		);
	} else if (strcmp(str, "func") == 0) {
		c = lvm_next_char_after_whitespaces(input);
		if (c != '(') {
			// Missing "(" after "func"
			return NULL;
		}
		
		lvm_atom_p args = lvm_pair_atom(lvm, lvm_nil_atom(lvm), lvm_nil_atom(lvm));
		lvm_atom_p current_arg = args;
		while (true) {
		 	c = lvm_next_char_after_whitespaces(input);
		 	if (c == ')')  // End of arg list
		 		break;
		 	else
		 		ungetc(c, input);
		 	
			current_arg->rest = lvm_pair_atom(lvm, lvm_c_read(lvm, input), current_arg->rest);
			current_arg = current_arg->rest;
			
			c = lvm_next_char_after_whitespaces(input);
		 	if (c == ',') {  // Next arg
		 		// Continue with next iteration
		 	} else if (c == ')') {
		 		break;
		 	} else {
		 		fprintf(stderr, "')' or ',' expected after argument name\n");
		 		return NULL;
		 	}
		}
		// Strip the first unnecessary nil, we just put it there so the loop doesn't
		// need a special case to append the first arg.
		args = args->rest;
		
		lvm_atom_p body = lvm_c_read(lvm, input);
		return lvm_pair_atom(lvm,
			lvm_sym_atom(lvm, "lambda"),
			lvm_pair_atom(lvm,
				args,
				lvm_pair_atom(lvm,
					body,
					lvm_nil_atom(lvm)
				)
			)
		);
	}
	
	// Got a valid symbol
	lvm_atom_p symbol = lvm_sym_atom(lvm, str);
	return symbol;
}

static int lvm_next_char_after_whitespaces(FILE* input) {
	int c;
	
	do {
		c = fgetc(input);
	} while ( isspace(c) );
	
	return c;
}

/*
void lvm_c_print(lvm_p lvm, FILE* output, lvm_atom_p atom) {
	switch(atom->type) {
		case LVM_T_NIL:
			fprintf(output, "nil");
			break;
		case LVM_T_TRUE:
			fprintf(output, "true");
			break;
		case LVM_T_FALSE:
			fprintf(output, "false");
			break;
		case LVM_T_NUM:
			fprintf(output, "%lu", (uint64_t)atom->num);
			break;
		case LVM_T_SYM:
			fprintf(output, "%s", atom->str);
			break;
		case LVM_T_STR:
			fprintf(output, "\"%s\"", atom->str);
			break;
		case LVM_T_PAIR:
			fprintf(output, "(");
			while(atom->type == LVM_T_PAIR) {
				lvm_print(lvm, output, atom->first);
				if (atom->rest->type != LVM_T_NIL && atom->rest->type != LVM_T_PAIR) {
					fprintf(output, " . ");
					lvm_print(lvm, output, atom->rest);
				} else if (atom->rest->type == LVM_T_PAIR) {
					fprintf(output, " ");
				}
				atom = atom->rest;
			}
			fprintf(output, ")");
			break;
		case LVM_T_BUILTIN:
			fprintf(output, "builtin(%p)", atom->builtin);
			break;
		case LVM_T_SYNTAX:
			fprintf(output, "syntax(%p)", atom->builtin);
			break;
		case LVM_T_LAMBDA:
			fprintf(output, "(lambda ");
			lvm_print(lvm, output, atom->args);
			fprintf(output, " ");
			for(lvm_atom_p expr = atom->body; expr->type == LVM_T_PAIR; expr = expr->rest) {
				lvm_print(lvm, output, expr->first);
				if (expr->rest->type == LVM_T_PAIR)
					fprintf(output, " ");
			}
			fprintf(output, ")");
			break;
		case LVM_T_ERROR:
			fprintf(output, "error(%s)\n", atom->str);
			break;
	}
}
*/