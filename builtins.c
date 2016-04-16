#include "common.h"


static lvm_atom_p plus(lvm_p lvm, lvm_atom_p args, lvm_env_p env) {
	if ( !(args->type == LVM_T_PAIR && args->rest->type == LVM_T_PAIR) ) {
		// TODO: use error atom
		fprintf(stderr, "lvm_plus(): first two args need to be pairs!\n");
		return lvm_nil_atom(lvm);
	}
	
	lvm_atom_p a = lvm_eval(lvm, args->first, env, stderr);
	lvm_atom_p b = lvm_eval(lvm, args->rest->first, env, stderr);
	if ( !(a->type == LVM_T_NUM && b->type == LVM_T_NUM) ) {
		// TODO: use error atom
		fprintf(stderr, "lvm_plus(): args need to be numbers!\n");
		return lvm_nil_atom(lvm);
	}
	
	return lvm_num_atom(lvm, a->num + b->num);
}


lvm_env_p lvm_new_base_env(lvm_p lvm) {
	lvm_env_p env = lvm_env_new(lvm, NULL);
	
	lvm_env_put(lvm, env, "+", lvm_builtin_atom(lvm, plus));
	
	return env;
}