#include <string.h>
#include <ctype.h>
#include "internals.h"

/** simple LISP syntax

3141 → LVM_T_NUM
"foo bar" → LVM_T_STR
(a b c) → LVM_T_PAIR
nil → LVM_T_NIL
true → LVM_T_TRUE
false → LVM_T_FALSE
_foo_bar → LVM_T_SYM

**/

static lvm_atom_p lvm_read_list(lvm_p lvm, FILE* input);
static int lvm_next_char_after_whitespaces(FILE* input);


lvm_atom_p lvm_read(lvm_p lvm, FILE* input) {
	int c = lvm_next_char_after_whitespaces(input);
	char* str = NULL;
	switch(c) {
		case EOF:
			return NULL;
		case '"':
			if ( fscanf(input, "%m[^\"]\"", &str) != 1 )
				return NULL;
			return lvm_str_atom(lvm, str);
		case '(':
			return lvm_read_list(lvm, input);
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
	if ( fscanf(input, " %m[^ \t\n()\"]", &str) != 1 )
		return NULL;
	
	if (strcmp(str, "nil") == 0)
		return lvm_nil_atom(lvm);
	else if (strcmp(str, "true") == 0)
		return lvm_true_atom(lvm);
	else if (strcmp(str, "false") == 0)
		return lvm_false_atom(lvm);
	return lvm_sym_atom(lvm, str);
}

static lvm_atom_p lvm_read_list(lvm_p lvm, FILE* input) {
	int c = lvm_next_char_after_whitespaces(input);
	switch(c) {
		case EOF:
			return NULL;
		case ')':
			return lvm_nil_atom(lvm);
		default:
			ungetc(c, input);
			lvm_atom_p first = lvm_read(lvm, input);
			lvm_atom_p rest = lvm_read_list(lvm, input);
			return lvm_pair_atom(lvm, first, rest);
	}
}

static int lvm_next_char_after_whitespaces(FILE* input) {
	int c;
	
	do {
		c = fgetc(input);
	} while ( isspace(c) );
	
	return c;
}


void lvm_print(lvm_p lvm, FILE* output, lvm_atom_p atom) {
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
			lvm_print(lvm, output, atom->body);
			fprintf(output, ")");
			break;
		case LVM_T_ERROR:
			fprintf(output, "error(%s)\n", atom->str);
			break;
	}
}