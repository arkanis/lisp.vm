// For strdup() in memory.h
#define _GNU_SOURCE
#include <stdlib.h>

#define SLIM_HASH_IMPLEMENTATION
#include "memory.h"

static lvm_atom_p alloc_atom(lvm_atom_t content) {
	lvm_atom_p atom = malloc(sizeof(lvm_atom_t));
	*atom = content;
	return atom;
}

lvm_atom_p lvm_num_atom(int64_t value) {
	return alloc_atom((lvm_atom_t){ .type = LVM_T_NUM, .num = value });
}

lvm_atom_p lvm_str_atom(char* value) {
	return alloc_atom((lvm_atom_t){ .type = LVM_T_STR, .str = value });
}

lvm_atom_p lvm_sym_atom(char* value) {
	return alloc_atom((lvm_atom_t){ .type = LVM_T_SYM, .str = value });
}

lvm_atom_p lvm_nil_atom() {
	return alloc_atom((lvm_atom_t){ .type = LVM_T_NIL });
}

lvm_atom_p lvm_true_atom() {
	return alloc_atom((lvm_atom_t){ .type = LVM_T_TRUE });
}

lvm_atom_p lvm_false_atom() {
	return alloc_atom((lvm_atom_t){ .type = LVM_T_FALSE });
}

lvm_atom_p lvm_pair_atom(lvm_atom_p first, lvm_atom_p rest) {
	lvm_atom_t pair = { .type = LVM_T_PAIR };
	pair.first = first;
	pair.rest = rest;
	return alloc_atom(pair);
}

lvm_atom_p lvm_builtin_atom(lvm_builtin_func_t func) {
	return alloc_atom((lvm_atom_t){ .type = LVM_T_BUILTIN, .builtin = func });
}


//
// Symbol table
//

SH_GEN_DICT_DEF(symtab , char*, lvm_atom_p);


//
// Environments
//

env_p env_new(env_p parent) {
	env_p env = malloc(sizeof(env_t));
	env->parent = parent;
	symtab_new(&env->bindings);
	return env;
}

void env_destroy(env_p env) {
	symtab_destroy(&env->bindings);
	free(env);
}

void env_put(env_p env, char* name, lvm_atom_p atom) {
	symtab_put(&env->bindings, name, atom);
}

lvm_atom_p env_get(env_p env, char* name) {
	do {
		lvm_atom_p value = symtab_get(&env->bindings, name, NULL);
		if (value)
			return value;
		env = env->parent;
	} while (env != NULL);
	
	return lvm_nil_atom();
}


//
// Eval argument stack
//

void lvm_eval_stack_push(lvm_p lvm, lvm_atom_p atom) {
	if (lvm->eval_stack_length == lvm->eval_stack_capacity) {
		lvm->eval_stack_capacity *= 2;
		lvm->eval_stack_ptr = realloc(lvm->eval_stack_ptr, lvm->eval_stack_capacity * sizeof(lvm->eval_stack_ptr[0]));
	}
	lvm->eval_stack_ptr[lvm->eval_stack_length++] = atom;
}

lvm_atom_p lvm_eval_stack_pop(lvm_p lvm) {
	return lvm->eval_stack_ptr[--lvm->eval_stack_length];
}