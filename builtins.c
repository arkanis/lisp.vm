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


static lvm_atom_p lvm_define(lvm_p lvm, lvm_atom_p args, lvm_env_p env) {
	if ( !(
		args->type == LVM_T_PAIR && args->first->type == LVM_T_SYM &&
		args->rest->type == LVM_T_PAIR && args->rest->rest->type == LVM_T_NIL
	) )
		return lvm_error_atom(lvm, "lvm_define(): first arg needs to be a symbol followed by an expr");
	//if (argc != 2 || argv[0]->type != LVM_T_SYM)
	//	return lvm_error_atom(lvm, "lvm_define(): first arg needs to be a symbol followed by an expr");
	lvm_atom_p name = args->first;
	lvm_atom_p value = lvm_eval(lvm, args->rest->first, env);
	lvm_env_put(lvm, env, name->str, value);
	return value;
}

static lvm_atom_p lvm_if(lvm_p lvm, lvm_atom_p args, lvm_env_p env) {
	//if ( !(argc == 2 || argc == 3) )
	//	return lvm_error_atom(lvm, "lvm_if(): if needs 2 or 3 args");
	lvm_atom_p evaled_condition = lvm_eval(lvm, args->first, env);
	lvm_atom_p result = lvm_nil_atom(lvm);
	if (evaled_condition->type == LVM_T_TRUE) {
		result = lvm_eval(lvm, args->rest->first, env);
	} else if (args->rest->rest->type == LVM_T_PAIR) {
		result = lvm_eval(lvm, args->rest->rest->first, env);
	}
	return result;
}

static lvm_atom_p lvm_lambda(lvm_p lvm, lvm_atom_p args, lvm_env_p env) {
	//if (argc != 2 || !(argv[0]->type == LVM_T_PAIR || argv[0]->type == LVM_T_NIL) || argv[1]->type != LVM_T_PAIR)
	//	return lvm_error_atom(lvm, "lvm_lambda(): first arg can be an empty list, second arg has to be an expr");
	return lvm_lambda_atom(lvm, args->first, args->rest->first);
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
	
	lvm_env_put(lvm, env, "define", lvm_syntax_atom(lvm, lvm_define));
	lvm_env_put(lvm, env, "if",     lvm_syntax_atom(lvm, lvm_if));
	lvm_env_put(lvm, env, "lambda", lvm_syntax_atom(lvm, lvm_lambda));
	
	return env;
}