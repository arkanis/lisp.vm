#include "common.h"

static lvm_atom_p lvm_eval_pair(lvm_p lvm, lvm_atom_p atom, lvm_env_p env);


lvm_atom_p lvm_eval(lvm_p lvm, lvm_atom_p atom, lvm_env_p env) {
	switch(atom->type) {
		case LVM_T_NIL:
		case LVM_T_TRUE:
		case LVM_T_FALSE:
		case LVM_T_NUM:
		case LVM_T_STR:
		case LVM_T_ERROR:
			return atom;
		case LVM_T_SYM: {
			lvm_atom_p binding = lvm_env_get(lvm, env, atom->str);
			return binding ? binding : lvm_error_atom(lvm, "lvm_eval(): no binding for symbol %s", atom->str);
			}
		case LVM_T_PAIR:
			return lvm_eval_pair(lvm, atom, env);
		case LVM_T_LAMBDA:
			return lvm_error_atom(lvm, "lvm_eval(): can't eval lambda!");
		case LVM_T_BUILTIN:
			return lvm_error_atom(lvm, "lvm_eval(): can't eval builtin!");
		case LVM_T_SYNTAX:
			return lvm_error_atom(lvm, "lvm_eval(): can't eval syntax!");
	}
	
	return NULL;
}

static lvm_atom_p lvm_eval_pair(lvm_p lvm, lvm_atom_p atom, lvm_env_p env) {
	// Eval first element of the list
	lvm_atom_p func = lvm_eval(lvm, atom->first, env);
	
	switch(func->type) {
		case LVM_T_BUILTIN: {
			// Eval all arguments and push them on the arg stack
			size_t prev_length = lvm->arg_stack_length;
			for(lvm_atom_p arg = atom->rest; arg->type == LVM_T_PAIR; arg = arg->rest) {
				lvm_atom_p evaled_arg = lvm_eval(lvm, arg->first, env);
				if (evaled_arg->type == LVM_T_ERROR) {
					// One argument evaled into an error atom. Stop evaling any arguments,
					// drop any args we already pushed and return this error atom. This
					// way we unwind the call stack.
					lvm_arg_stack_drop(lvm, lvm->arg_stack_length - prev_length);
					return evaled_arg;
				}
				lvm_arg_stack_push(lvm, evaled_arg);
			}
			
			// Call builtin and drop arguments from the arg stack
			size_t arg_count = lvm->arg_stack_length - prev_length;
			lvm_atom_p result = func->builtin(lvm, arg_count, lvm->arg_stack_ptr + prev_length, env);
			lvm_arg_stack_drop(lvm, arg_count);
			return result;
		}
		case LVM_T_SYNTAX:
			return func->syntax(lvm, atom->rest, env);
		case LVM_T_LAMBDA: {
			lvm_env_p lambda_env = lvm_env_new(lvm, env);
			
			lvm_atom_p arg_name = func->args;
			lvm_atom_p arg_value = atom->rest;
			while (arg_name->type == LVM_T_PAIR && arg_value->type == LVM_T_PAIR) {
				lvm_atom_p evaled_arg_value = lvm_eval(lvm, arg_value->first, env);
				lvm_env_put(lvm, lambda_env, arg_name->first->str, evaled_arg_value);
				
				arg_name = arg_name->rest;
				arg_value = arg_value->rest;
			}
			
			return lvm_eval(lvm, func->body, lambda_env);
		}
		default:
			return lvm_error_atom(lvm, "lvm_eval_pair(): got wrong atom in function slot!");
	}
}