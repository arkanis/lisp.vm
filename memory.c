// For strdup() in slim_hash.h
#define _GNU_SOURCE
#include <string.h>
#include "common.h"

typedef struct lvm_atom_s lvm_atom_t;

static lvm_atom_p lvm_alloc_atom(lvm_p lvm, lvm_atom_t content);


//
// Init stuff
//

void lvm_init_base_atoms(lvm_p lvm) {
	lvm->nil_atom   = lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_NIL   });
	lvm->true_atom  = lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_TRUE  });
	lvm->false_atom = lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_FALSE });
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

lvm_atom_p lvm_builtin_atom(lvm_p lvm, lvm_builtin_func_t func) {
	return lvm_alloc_atom(lvm, (lvm_atom_t){ .type = LVM_T_BUILTIN, .builtin = func });
}


//
// Environment stuff
//

#define SLIM_HASH_IMPLEMENTATION
#include "slim_hash.h"

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
	
	// TODO: use error atom with binding not found error message
	return lvm_nil_atom(lvm);
}