#include "common.h"

lvm_p lvm_new() {
	lvm_p lvm = malloc(sizeof(lvm_t));
	if (lvm == NULL)
		return NULL;
	
	lvm_dict_new(&lvm->symbol_table);
	lvm_init_base_atoms(lvm);
	lvm->base_env = lvm_new_base_env(lvm);
	lvm->alloced_atoms = 0;
	
	return lvm;
}

void lvm_destroy(lvm_p lvm) {
	lvm_dict_destroy(&lvm->symbol_table);
	free(lvm);
}

lvm_env_p lvm_base_env(lvm_p lvm) {
	return lvm->base_env;
}