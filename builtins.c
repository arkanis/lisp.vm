#include <string.h>
#include "internals.h"


//
// Builtins
//

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


static lvm_atom_p lvm_eq(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	if (argc != 2)
		return lvm_error_atom(lvm, "lvm_eq(): unsupported args");
	lvm_atom_p a = argv[0], b = argv[1];
	if (a == b) {
		return lvm_true_atom(lvm);
	} else if (a->type == LVM_T_NUM && b->type == LVM_T_NUM) {
		return (a->num == b->num) ? lvm_true_atom(lvm) : lvm_false_atom(lvm);
	} else if (a->type == LVM_T_STR && b->type == LVM_T_STR) {
		return strcmp(a->str, b->str) == 0 ? lvm_true_atom(lvm) : lvm_false_atom(lvm);
	}
	return lvm_false_atom(lvm);
}

static lvm_atom_p lvm_lt(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	if (argc != 2 || argv[0]->type != LVM_T_NUM || argv[1]->type != LVM_T_NUM)
		return lvm_error_atom(lvm, "lvm_lt(): supports only two numbers");
	return (argv[0]->num < argv[1]->num) ? lvm_true_atom(lvm) : lvm_false_atom(lvm);
}

static lvm_atom_p lvm_gt(lvm_p lvm, size_t argc, lvm_atom_p argv[], lvm_env_p env) {
	if (argc != 2 || argv[0]->type != LVM_T_NUM || argv[1]->type != LVM_T_NUM)
		return lvm_error_atom(lvm, "lvm_gt(): supports only two numbers");
	return (argv[0]->num > argv[1]->num) ? lvm_true_atom(lvm) : lvm_false_atom(lvm);
}


//
// Syntax builtins
//

static bool lvm_verify_syn_arg_count(lvm_atom_p args, uint32_t count, bool can_take_more_args) {
	lvm_atom_p arg = args;
	for(uint32_t i = 0; i < count; i++) {
		if (arg->type != LVM_T_PAIR)
			return false;
		arg = arg->rest;
	}
	
	if ( !can_take_more_args && arg->type != LVM_T_NIL )
		return false;
	return true;
}

static lvm_atom_p lvm_define(lvm_p lvm, lvm_atom_p args, lvm_env_p env) {
	if ( !( lvm_verify_syn_arg_count(args, 2, false) && args->first->type == LVM_T_SYM ) )
		return lvm_error_atom(lvm, "lvm_define(): first arg needs to be a symbol followed by an expr");
	lvm_atom_p name = args->first;
	lvm_atom_p value = lvm_eval(lvm, args->rest->first, env);
	if (value->type != LVM_T_ERROR)
		lvm_env_put(lvm, env, name->str, value);
	return value;
}

static lvm_atom_p lvm_if(lvm_p lvm, lvm_atom_p args, lvm_env_p env) {
	if ( !( lvm_verify_syn_arg_count(args, 2, false) || lvm_verify_syn_arg_count(args, 3, false) ) )
		return lvm_error_atom(lvm, "lvm_if(): if needs 2 or 3 args");
	lvm_atom_p evaled_condition = lvm_eval(lvm, args->first, env);
	if (evaled_condition->type == LVM_T_ERROR)
		return evaled_condition;
	
	lvm_atom_p result = lvm_nil_atom(lvm);
	if (evaled_condition->type == LVM_T_TRUE) {
		result = lvm_eval(lvm, args->rest->first, env);
	} else if (args->rest->rest->type == LVM_T_PAIR) {
		result = lvm_eval(lvm, args->rest->rest->first, env);
	}
	return result;
}

static lvm_atom_p lvm_lambda(lvm_p lvm, lvm_atom_p args, lvm_env_p env) {
	if ( !( lvm_verify_syn_arg_count(args, 2, true) && args->first->type == LVM_T_PAIR ) )
		return lvm_error_atom(lvm, "lvm_lambda(): first arg has to be a list followed by one or more expressions");
	return lvm_lambda_atom(lvm, args->first, args->rest);
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
	
	lvm_env_put(lvm, env, "=", lvm_builtin_atom(lvm, lvm_eq));
	lvm_env_put(lvm, env, "<", lvm_builtin_atom(lvm, lvm_lt));
	lvm_env_put(lvm, env, ">", lvm_builtin_atom(lvm, lvm_gt));
	
	lvm_env_put(lvm, env, "define", lvm_syntax_atom(lvm, lvm_define));
	lvm_env_put(lvm, env, "if",     lvm_syntax_atom(lvm, lvm_if));
	lvm_env_put(lvm, env, "lambda", lvm_syntax_atom(lvm, lvm_lambda));
	
	return env;
}