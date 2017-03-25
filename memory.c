// For strdup() in slim_hash.h
#define _GNU_SOURCE
#include <string.h>
#include <stdarg.h>
#include "internals.h"

typedef struct lvm_atom_s lvm_atom_t;

static lvm_atom_p lvm_alloc_atom(lvm_p lvm, lvm_atom_t content);


//
// Init stuff
//

void lvm_mem_init(lvm_p lvm) {
	lvm_dict_new(&lvm->symbol_table);
	
#	ifdef GC_REGION_BAKER
#	else
	lvm->nil_atom   = lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_NIL   });
	lvm->true_atom  = lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_TRUE  });
	lvm->false_atom = lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_FALSE });
#	endif
	
	lvm->arg_stack_length = 0;
	lvm->arg_stack_capacity = 16;
	lvm->arg_stack_ptr = malloc(lvm->arg_stack_capacity * sizeof(lvm->arg_stack_ptr[0]));
	
	lvm->alloced_atoms = 0;
}

void lvm_mem_free(lvm_p lvm) {
	free(lvm->arg_stack_ptr);
}


//
// Atom memory management
//

static lvm_atom_p lvm_alloc_atom(lvm_p lvm, lvm_atom_t content) {
	lvm_atom_p atom = malloc(sizeof(lvm_atom_t));
	*atom = content;
	lvm->alloced_atoms++;
	return atom;
}

lvm_atom_p lvm_nil_atom(lvm_p lvm) {
	return lvm->nil_atom;
}

lvm_atom_p lvm_true_atom(lvm_p lvm) {
	return lvm->true_atom;
}

lvm_atom_p lvm_false_atom(lvm_p lvm) {
	return lvm->false_atom;
}

lvm_atom_p lvm_num_atom(lvm_p lvm, int64_t value) {
	return lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_NUM, .num = value });
}

lvm_atom_p lvm_sym_atom(lvm_p lvm, char* value) {
	lvm_atom_p symbol = lvm_dict_get(&lvm->symbol_table, value, NULL);
	if (!symbol) {
		symbol = lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_SYM, .str = value });
		lvm_dict_put(&lvm->symbol_table, value, symbol);
	}
	return symbol;
}

lvm_atom_p lvm_str_atom(lvm_p lvm, char* value) {
	return lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_STR, .str = value });
}

lvm_atom_p lvm_pair_atom(lvm_p lvm, lvm_atom_p first, lvm_atom_p rest) {
	lvm_atom_t pair = { .type = LVM_T_PAIR };
	pair.first = first;
	pair.rest = rest;
	return lvm_alloc_atom(lvm, pair);
}

lvm_atom_p lvm_lambda_atom(lvm_p lvm, lvm_atom_p args, lvm_atom_p body, lvm_env_p env) {
	lvm_atom_t lambda = { .type = LVM_T_LAMBDA };
	lambda.args = args;
	lambda.body = body;
	lambda.env = env;
	return lvm_alloc_atom(lvm, lambda);
}

lvm_atom_p lvm_builtin_atom(lvm_p lvm, lvm_builtin_func_t func) {
	return lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_BUILTIN, .builtin = func });
}

lvm_atom_p lvm_syntax_atom(lvm_p lvm, lvm_syntax_func_t func) {
	return lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_SYNTAX, .syntax = func });
}

lvm_atom_p lvm_error_atom(lvm_p lvm, const char* format, ...) {
	va_list args;
	va_start(args, format);
		char* message = NULL;
		vasprintf(&message, format, args);
	va_end(args);
	
	return lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_ERROR, .str = message });
}


//
// Environment stuff
//

#define SLIM_HASH_IMPLEMENTATION
#include "slim_hash.h"

#define SLIM_HASH_CALLOC lvm_gc_calloc
#define SLIM_HASH_FREE   lvm_gc_free
SH_GEN_DICT_DEF(lvm_dict, const char*, lvm_atom_p);


lvm_env_p lvm_env_new(lvm_p lvm, lvm_env_p parent) {
	lvm_env_p env = malloc(sizeof(lvm_env_t));
	env->parent = parent;
	lvm_dict_new(&env->bindings);
	return env;
}

void lvm_env_destroy(lvm_p lvm, lvm_env_p env) {
	lvm_dict_destroy(&env->bindings);
	free(env);
}

void lvm_env_put(lvm_p lvm, lvm_env_p env, char* name, lvm_atom_p atom) {
	lvm_dict_put(&env->bindings, name, atom);
}

lvm_atom_p lvm_env_get(lvm_p lvm, lvm_env_p env, char* name) {
	do {
		lvm_atom_p value = lvm_dict_get(&env->bindings, name, NULL);
		if (value)
			return value;
		env = env->parent;
	} while (env != NULL);
	
	return NULL;
}


//
// Eval argument stack stuff
//


void lvm_arg_stack_push(lvm_p lvm, lvm_atom_p atom) {
	if (lvm->arg_stack_length >= lvm->arg_stack_capacity) {
		lvm->arg_stack_capacity *= 2;
		lvm->arg_stack_ptr = realloc(lvm->arg_stack_ptr, lvm->arg_stack_capacity * sizeof(lvm->arg_stack_ptr[0]));
	}
	
	lvm->arg_stack_ptr[lvm->arg_stack_length++] = atom;
}

void lvm_arg_stack_drop(lvm_p lvm, size_t count) {
	if (count > lvm->arg_stack_length) {
		fprintf(stderr, "trying to drop more atoms from the arg stack than there are! eval might be broken!\n");
		abort();
	}
	
	lvm->arg_stack_length -= count;
}