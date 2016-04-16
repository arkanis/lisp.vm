#include <stdbool.h>
#include <assert.h>

#include <stdlib.h>

#include <sys/mman.h>

#include "interpreter.h"

interpreter_p interpreter_new(size_t stack_size){
	interpreter_p interp = malloc(sizeof(interpreter_t));
	interp->stack_size = stack_size;
	interp->stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	return interp;
}

void interpreter_destroy(interpreter_p interp){
	munmap(interp->stack, interp->stack_size);
	interp->stack = NULL;
	interp->stack_size = 0;
	free(interp);
}

/**
 * Executes the specified module in the interpreter. Execution starts at the first
 * function of the module.
 */
atom_t interpreter_exec(interpreter_p interp, module_p mod){
	assert(mod->function_count >= 1);
	func_p func = mod->functions;
	atom_p fp = interp->stack;
	atom_p sp = interp->stack;
	
	for(instruction_p ip = func->instructions; assert(ip - func->instructions < func->instruction_count), true; ip++){
		switch(ip->op){
			case BC_LOAD_NIL:
				sp->type = T_NIL;
				sp++;
				break;
			case BC_LOAD_TRUE:
				sp->type = T_TRUE;
				sp++;
				break;
			case BC_LOAD_FALSE:
				sp->type = T_FALSE;
				sp++;
				break;
			case BC_LOAD_INT:  // num
				*sp = int_atom(ip->num);
				sp++;
				break;
			case BC_LOAD_LITERAL:  // index
				assert(ip->index < mod->literal_count);
				*sp = mod->literals[ip->index];
				sp++;
				break;
			case BC_LOAD_FUNC:  // index
				assert(ip->index < mod->function_count);
				*sp = func_atom(ip->index, fp);
				sp++;
				break;
			
			case BC_LOAD_ARG:  // scope_offset, index
				assert(0);
				break;
			case BC_LOAD_LOCAL:  // scope_offset, index
				assert(0);
				break;
			case BC_STORE_LOCAL:  // scope_offset, index
				assert(0);
				break;
			
			case BC_DROP:  // count
				sp--;
				break;
			case BC_CALL:  // count
				assert(0);
				break;
			case BC_RETURN:  // count?
				sp--;
				return *sp;
				break;
			
			case BC_JUMP:  // jump_offset
				ip += ip->jump_offset;
				break;
			case BC_JUMP_IF_FALSE: // jump_offset
				sp--;
				if (sp->type == T_FALSE)
					ip += ip->jump_offset;
				break;
			
			case BC_PACK:  // count
				*sp = array_atom(ip->count, sp - ip->count);
				sp++;
				break;
			case BC_EXTRACT:  // index
				sp--;
				assert(sp->type == T_ARRAY && ip->index < sp->len);
				*sp = sp->atoms[ip->index];
				sp++;
				break;
			
			case BC_ADD: case BC_SUB: case BC_MUL: case BC_DIV:
			case BC_EQ: case BC_LT: case BC_GT:
			{
				atom_t left, right;
				
				// Load operator from stack or from the instruction if specified
				switch(ip->operand){
					case BC_OP_NONE:
						sp--;
						right = *sp;
						sp--;
						left = *sp;
						break;
					case BC_OP_LEFT:
						left = int_atom(ip->num);
						sp--;
						right = *sp;
						break;
					case BC_OP_RIGHT:
						sp--;
						left = *sp;
						right = int_atom(ip->num);
						break;
				}
				
				// Convert both operands to float if one of them is a float and the other int
				if (left.type == T_INT && right.type == T_FLOAT)
					left = float_atom(left.inum);
				if (left.type == T_FLOAT && right.type == T_INT)
					right = float_atom(right.inum);
				
				switch(ip->op){
					case BC_ADD:
						*sp = (left.type == T_INT) ? int_atom(left.inum + right.inum) : float_atom(left.fnum + right.fnum);
						break;
					case BC_SUB:
						*sp = (left.type == T_INT) ? int_atom(left.inum - right.inum) : float_atom(left.fnum - right.fnum);
						break;
					case BC_MUL:
						*sp = (left.type == T_INT) ? int_atom(left.inum * right.inum) : float_atom(left.fnum * right.fnum);
						break;
					case BC_DIV:
						*sp = (left.type == T_INT) ? int_atom(left.inum / right.inum) : float_atom(left.fnum / right.fnum);
						break;
					
					case BC_EQ:
						*sp = atoms_equal(left, right) ? true_atom() : false_atom();
						break;
					case BC_LT:
						assert(left.type == right.type && (left.type == T_INT || left.type == T_FLOAT));
						if (left.type == T_INT)
							*sp = (left.inum < right.inum) ? true_atom() : false_atom();
						else
							*sp = (left.fnum < right.fnum) ? true_atom() : false_atom();
						break;
					case BC_GT:
						assert(left.type == right.type && (left.type == T_INT || left.type == T_FLOAT));
						if (left.type == T_INT)
							*sp = (left.inum > right.inum) ? true_atom() : false_atom();
						else
							*sp = (left.fnum > right.fnum) ? true_atom() : false_atom();
						break;
				}
				sp++;
				
			}	break;
		}
	}
	
	return (atom_t){T_NIL};
}