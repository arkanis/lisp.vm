#include "common.h"


static lvm_atom_p lvm_add(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	if (argc != 2 || argv[0]->type != LVM_T_NUM || argv[1]->type != LVM_T_NUM)
		return lvm_error_atom(lvm, "lvm_add(): supports only two number arguments");
	return lvm_num_atom(lvm, argv[0]->num + argv[1]->num);
}

static lvm_atom_p lvm_sub(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	if (argc != 2 || argv[0]->type != LVM_T_NUM || argv[1]->type != LVM_T_NUM)
		return lvm_error_atom(lvm, "lvm_sub(): supports only two number arguments");
	return lvm_num_atom(lvm, argv[0]->num - argv[1]->num);
}

static lvm_atom_p lvm_mul(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	if (argc != 2 || argv[0]->type != LVM_T_NUM || argv[1]->type != LVM_T_NUM)
		return lvm_error_atom(lvm, "lvm_mul(): supports only two number arguments");
	return lvm_num_atom(lvm, argv[0]->num * argv[1]->num);
}

static lvm_atom_p lvm_div(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	if (argc != 2 || argv[0]->type != LVM_T_NUM || argv[1]->type != LVM_T_NUM)
		return lvm_error_atom(lvm, "lvm_div(): supports only two number arguments");
	return lvm_num_atom(lvm, argv[0]->num / argv[1]->num);
}


static lvm_atom_p lvm_cons(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	if (argc != 2)
		return lvm_error_atom(lvm, "lvm_cons(): supports only two arguments");
	return lvm_pair_atom(lvm, argv[0], argv[1]);
}

static lvm_atom_p lvm_first(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	if (argc != 1 || argv[0]->type != LVM_T_PAIR)
		return lvm_error_atom(lvm, "lvm_first(): supports only one pair argument");
	return argv[0]->first;
}

static lvm_atom_p lvm_rest(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	if (argc != 1 || argv[0]->type != LVM_T_PAIR)
		return lvm_error_atom(lvm, "lvm_rest(): supports only one pair argument");
	return argv[0]->rest;
}


lvm_env_p lvm_new_base_env(lvm_p lvm) {
	lvm_env_p env = lvm_env_new(lvm, NULL);
	
	lvm_env_put(lvm, env, "+", lvm_builtin_atom(lvm, lvm_add));
	lvm_env_put(lvm, env, "-", lvm_builtin_atom(lvm, lvm_sub));
	lvm_env_put(lvm, env, "*", lvm_builtin_atom(lvm, lvm_mul));
	lvm_env_put(lvm, env, "/", lvm_builtin_atom(lvm, lvm_div));
	
	lvm_env_put(lvm, env, "cons",  lvm_builtin_atom(lvm, lvm_cons));
	lvm_env_put(lvm, env, "first", lvm_builtin_atom(lvm, lvm_first));
	lvm_env_put(lvm, env, "rest",  lvm_builtin_atom(lvm, lvm_rest));
	
	return env;
}