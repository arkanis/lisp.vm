#include "common.h"

static lvm_atom_p lvm_eval_pair(lvm_p lvm, lvm_atom_p atom, lvm_env_p env, FILE* errors);


lvm_atom_p lvm_eval(lvm_p lvm, lvm_atom_p atom, lvm_env_p env, FILE* errors) {
	switch(atom->type) {
		case LVM_T_NIL:
		case LVM_T_TRUE:
		case LVM_T_FALSE:
		case LVM_T_NUM:
		case LVM_T_STR:
		case LVM_T_BUILTIN:
			return atom;
		case LVM_T_SYM:
			return lvm_env_get(lvm, env, atom->str);
		case LVM_T_PAIR:
			return lvm_eval_pair(lvm, atom, env, errors);
	}
	
	return NULL;
}

static lvm_atom_p lvm_eval_pair(lvm_p lvm, lvm_atom_p atom, lvm_env_p env, FILE* errors) {
	// Eval first element of the list
	lvm_atom_p func = lvm_eval(lvm, atom->first, env, errors);
	if (func->type != LVM_T_BUILTIN) {
		// TODO: use error atom for error message
		fprintf(errors, "lvm_eval_pair(): no builtin in function slot: ");
		lvm_print(lvm, atom->first, errors);
		fprintf(errors, "\n");
		return lvm_nil_atom(lvm);
	}
	
	return func->builtin(lvm, atom->rest, env);
}