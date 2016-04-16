#pragma once
#include "slim_hash.h"
#include "lvm.h"


// A hashtable from string to atom
SH_GEN_DECL(lvm_dict, const char*, lvm_atom_p);


//
// General interpreter stuff
//

typedef struct lvm_s lvm_t;

struct lvm_s {
	lvm_atom_p nil_atom, true_atom, false_atom;
	lvm_env_p base_env;
	lvm_dict_t symbol_table;
	
	lvm_atom_p* arg_stack_ptr;
	size_t arg_stack_length, arg_stack_capacity;
	
	size_t alloced_atoms;
};


//
// Memory stuff
// 
// Details of struct lvm_atom_s are private to memory.c so we don't define
// lvm_atom_t here. This way everyone except memory.c can only use pointers to
// atoms.
//

void lvm_mem_init(lvm_p lvm);
void lvm_mem_free(lvm_p lvm);

void lvm_arg_stack_push(lvm_p lvm, lvm_atom_p atom);
void lvm_arg_stack_drop(lvm_p lvm, size_t count);


//
// Environment stuff
//

typedef struct lvm_env_s lvm_env_t;

struct lvm_env_s {
	lvm_env_p parent;
	lvm_dict_t bindings;
};

lvm_env_p lvm_new_base_env(lvm_p lvm);