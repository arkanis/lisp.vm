#pragma once

#include <stdint.h>
#include <stdio.h>


//
// General opaque pointer types and interpreter functions
//

typedef struct lvm_s *lvm_p;
typedef struct lvm_atom_s *lvm_atom_p;
typedef struct lvm_env_s *lvm_env_p;

lvm_p lvm_new();
void  lvm_destroy(lvm_p lvm);

lvm_env_p  lvm_base_env(lvm_p lvm);
lvm_atom_p lvm_read(lvm_p lvm, FILE* input, FILE* errors);
lvm_atom_p lvm_eval(lvm_p lvm, lvm_atom_p atom, lvm_env_p env, FILE* errors);
void       lvm_print(lvm_p lvm, lvm_atom_p atom, FILE* output);


//
// Atom types and allocation functions
//

typedef enum {
	LVM_T_NIL,
	LVM_T_TRUE,
	LVM_T_FALSE,
	LVM_T_NUM,
	LVM_T_SYM,
	LVM_T_STR,
	LVM_T_PAIR,
	LVM_T_BUILTIN
} lvm_atom_type_t;

typedef lvm_atom_p (*lvm_builtin_func_t)(lvm_p lvm, lvm_atom_p args, lvm_env_p env);

struct lvm_atom_s {
	lvm_atom_type_t type;
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

lvm_atom_p lvm_nil_atom(lvm_p lvm);
lvm_atom_p lvm_true_atom(lvm_p lvm);
lvm_atom_p lvm_false_atom(lvm_p lvm);
lvm_atom_p lvm_num_atom(lvm_p lvm, int64_t value);
lvm_atom_p lvm_sym_atom(lvm_p lvm, char* value);
lvm_atom_p lvm_str_atom(lvm_p lvm, char* value);
lvm_atom_p lvm_pair_atom(lvm_p lvm, lvm_atom_p first, lvm_atom_p rest);
lvm_atom_p lvm_builtin_atom(lvm_p lvm, lvm_builtin_func_t func);


//
// Environment functions
//

lvm_env_p  lvm_env_new(lvm_p lvm, lvm_env_p parent);
void       lvm_env_destroy(lvm_p lvm, lvm_env_p env);
void       lvm_env_put(lvm_p lvm, lvm_env_p env, char* name, lvm_atom_p atom);
lvm_atom_p lvm_env_get(lvm_p lvm, lvm_env_p env, char* name);