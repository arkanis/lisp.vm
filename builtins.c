#include "common.h"


static lvm_atom_p lvm_plus(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	if (argc != 2 || argv[0]->type != LVM_T_NUM || argv[1]->type != LVM_T_NUM) {
		// TODO: use error atom
		fprintf(stderr, "lvm_plus(): only two num args supported!\n");
		return lvm_nil_atom(lvm);
	}
	
	return lvm_num_atom(lvm, argv[0]->num + argv[1]->num);
}


lvm_env_p lvm_new_base_env(lvm_p lvm) {
	lvm_env_p env = lvm_env_new(lvm, NULL);
	
	lvm_env_put(lvm, env, "+", lvm_builtin_atom(lvm, lvm_plus));
	
	return env;
}