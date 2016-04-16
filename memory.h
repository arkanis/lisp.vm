#pragma once

#include <stdint.h>
#include <string.h>

#include "slim_hash.h"


#define LVM_T_NIL     0
#define LVM_T_TRUE    1
#define LVM_T_FALSE   2
#define LVM_T_NUM     3
#define LVM_T_SYM     4
#define LVM_T_STR     5
#define LVM_T_PAIR    6
#define LVM_T_BUILTIN 7

typedef struct lvm_atom_s lvm_atom_t, *lvm_atom_p;
typedef struct env_s env_t, *env_p;

//
// Atoms
//

typedef lvm_atom_p (*lvm_builtin_func_t)(lvm_atom_p args, env_p env);
struct lvm_atom_s {
	uint8_t type;
	uint8_t padding1, padding2, padding3;
	
	union {
		int64_t num;
		char* str;
		struct {
			lvm_atom_p first;
			lvm_atom_p rest;
		};
		lvm_builtin_func_t builtin;
	};
};

lvm_atom_p lvm_num_atom(int64_t value);
lvm_atom_p lvm_str_atom(char* value);
lvm_atom_p lvm_sym_atom(char* value);
lvm_atom_p lvm_nil_atom();
lvm_atom_p lvm_true_atom();
lvm_atom_p lvm_false_atom();
lvm_atom_p lvm_pair_atom(lvm_atom_p first, lvm_atom_p rest);
lvm_atom_p lvm_builtin_atom(lvm_builtin_func_t func);


//
// Symbol table
//


SH_GEN_DECL(symtab, char*, lvm_atom_p);

typedef struct {
	symtab_t symbols;
	lvm_atom_p* eval_stack_ptr;
	size_t eval_stack_length, eval_stack_capacity;
} lvm_t, *lvm_p;


//
// Environments
//

struct env_s {
	env_p parent;
	symtab_t bindings;
};

env_p env_new(env_p parent);
void  env_destroy(env_p env);

void       env_put(env_p env, char* name, lvm_atom_p atom);
lvm_atom_p env_get(env_p env, char* name);


//
// Eval argument stack
//

void       lvm_eval_stack_push(lvm_p lvm, lvm_atom_p atom);
lvm_atom_p lvm_eval_stack_pop(lvm_p lvm);