#pragma once
#include "slim_hash.h"
#include "lvm.h"


// Details of struct lvm_atom_s are private to memory.c


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
	size_t alloced_atoms;
};

void lvm_init_base_atoms(lvm_p lvm);


//
// Environment stuff
//

typedef struct lvm_env_s lvm_env_t;

struct lvm_env_s {
	lvm_env_p parent;
	lvm_dict_t bindings;
};

lvm_env_p lvm_new_base_env(lvm_p lvm);