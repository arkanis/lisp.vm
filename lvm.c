#include "lvm.h"

void lvm_new(lvm_p lvm) {
	symtab_new(&lvm->symbols);
	lvm->eval_stack_length = 0;
	lvm->eval_stack_capacity = 16;
	lvm->eval_stack_ptr = malloc(lvm->eval_stack_capacity * sizeof(lvm_atom_p));
}

void lvm_destroy(lvm_p lvm) {
	free(lvm->eval_stack_ptr);
	symtab_destroy(&lvm->symbols);
}