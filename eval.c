#include "common.h"

static lvm_atom_p lvm_eval_pair(lvm_p lvm, lvm_atom_p atom, lvm_env_p env, FILE* errors);


lvm_atom_p lvm_eval(lvm_p lvm, lvm_atom_p atom, lvm_env_p env, FILE* errors) {
	switch(atom->type) {
		case LVM_T_NIL:
		case LVM_T_TRUE:
		case LVM_T_FALSE:
		case LVM_T_NUM:
		case LVM_T_STR:
		// TODO: eval builtins int an error atom
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
	
	// Eval all arguments and push them on the arg stack
	size_t prev_length = lvm->arg_stack_length;
	for(lvm_atom_p arg = atom->rest; arg->type == LVM_T_PAIR; arg = arg->rest)
		lvm_arg_stack_push(lvm, lvm_eval(lvm, arg->first, env, errors));
	size_t arg_count = lvm->arg_stack_length - prev_length;
	
	// Call builtin and drop arguments from the arg stack
	lvm_atom_p result = func->builtin(lvm, arg_count, lvm->arg_stack_ptr + prev_length, env);
	lvm_arg_stack_drop(lvm, arg_count);
	
	return result;
}