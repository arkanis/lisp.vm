#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "memory.h"
#include "syntax.h"
#include "eval.h"
#include "lvm.h"

lvm_atom_p lvm_plus(lvm_atom_p args, env_p env) {
	if ( !(args->type == LVM_T_PAIR && args->rest->type == LVM_T_PAIR) ) {
		fprintf(stderr, "lvm_plus(): first two args need to be pairs!\n");
		return lvm_nil_atom();
	}
	
	lvm_atom_p a = lvm_eval(args->first, env);
	lvm_atom_p b = lvm_eval(args->rest->first, env);
	if ( !(a->type == LVM_T_NUM && b->type == LVM_T_NUM) ) {
		fprintf(stderr, "lvm_plus(): args need to be numbers!\n");
		return lvm_nil_atom();
	}
	
	return lvm_num_atom(a->num + b->num);
}

int main() {
	lvm_t lvm;
	lvm_new(&lvm);
	env_p env = env_new(NULL);
	env_put(env, "a", lvm_num_atom(37));
	env_put(env, "foo", lvm_num_atom(15));
	env_p child = env_new(env);
	env_put(child, "a", lvm_num_atom(17));
	env_put(child, "bar", lvm_num_atom(16));
	env_put(child, "+", lvm_builtin_atom(lvm_plus));
	
	while(1) {
		printf("> ");
		lvm_atom_p ast = lvm_read(stdin, stderr, &lvm);
		if (ast == NULL)
			break;
		lvm_atom_p result = lvm_eval(ast, child);
		lvm_print(result, stdout);
		printf("\n");
	}
	printf("\n");
	
	env_destroy(child);
	env_destroy(env);
	lvm_destroy(&lvm);
	return 0;
}