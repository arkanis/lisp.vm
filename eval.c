#include <stdio.h>
#include "eval.h"
#include "syntax.h"


static lvm_atom_p lvm_eval_pair(lvm_atom_p atom, env_p env);


lvm_atom_p lvm_eval(lvm_atom_p atom, env_p env) {
	switch(atom->type) {
		case LVM_T_NIL:
		case LVM_T_TRUE:
		case LVM_T_FALSE:
		case LVM_T_NUM:
		case LVM_T_STR:
			return atom;
		case LVM_T_SYM:
			return env_get(env, atom->str);
		case LVM_T_PAIR:
			return lvm_eval_pair(atom, env);
	}
	
	return NULL;
}

static lvm_atom_p lvm_eval_pair(lvm_atom_p atom, env_p env) {
	// Eval first element of the list
	lvm_atom_p func = lvm_eval(atom->first, env);
	if (func->type != LVM_T_BUILTIN) {
		fprintf(stderr, "lvm_eval_pair(): no builtin in function slot: ");
		lvm_print(atom->first, stderr);
		fprintf(stderr, "\n");
		return lvm_nil_atom();
	}
	
	return func->builtin(atom->rest, env);
}