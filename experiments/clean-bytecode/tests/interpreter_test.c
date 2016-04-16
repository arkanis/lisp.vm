#include "test_utils.h"
#include "test_helpers.h"
#include "../interpreter.h"

interpreter_p interp = NULL;

//
// Utility constructors
//

func_t sample_func(instruction_t instructions[]){
	func_t func = (func_t){
		.instructions = instructions,
		.instruction_count = 0,
		.arg_count = 0,
		.var_count = 0,
		.names = NULL
	};
	while(instructions[func.instruction_count].op != BC_EOL)
		func.instruction_count++;
	return func;
}


//
// Local check functions
//

void check_simple_func_exec(instruction_t instructions[], atom_t expected_result){
	func_t func = (func_t){
		.instructions = instructions,
		.instruction_count = 0,
		.arg_count = 0,
		.var_count = 0,
		.names = NULL
	};
	while(instructions[func.instruction_count].op != BC_EOL)
		func.instruction_count++;
	
	module_t mod = (module_t){
		.symbols = NULL,
		.symbol_count = 0,
		.literals = NULL,
		.literal_count = 0,
		.functions = &func,
		.function_count = 1
	};
	
	atom_t actual_result = interpreter_exec(interp, &mod);
	check_atom(actual_result, expected_result, &mod);
}

void check_mod_exec(module_p mod, atom_t expected_result){
	atom_t actual_result = interpreter_exec(interp, mod);
	check_atom(actual_result, expected_result, mod);
}


//
// Tests
//

void direct_load_instruction_test(){
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_NIL},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, nil_atom());
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_TRUE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, true_atom());
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_FALSE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, false_atom());
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 17},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(17));
}

void drop_instruction_test(){
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_TRUE},
		(instruction_t){BC_LOAD_FALSE},
		(instruction_t){BC_DROP},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, true_atom());
}

void load_literal_test(){
	func_t func = sample_func((instruction_t[]){
		(instruction_t){BC_LOAD_LITERAL, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	});
	module_t mod = (module_t){
		.literals = (atom_t[]){
			true_atom()
		},
		.literal_count = 1,
		.functions = &func,
		.function_count = 1
	};
	
	check_mod_exec(&mod, true_atom());
}

void jump_instruction_for_if_test(){
	// if (true) 42 else 17
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_TRUE},
		(instruction_t){BC_JUMP_IF_FALSE, .jump_offset = 2},
		(instruction_t){BC_LOAD_INT, .num = 42},
		(instruction_t){BC_JUMP, .jump_offset = 1},
		(instruction_t){BC_LOAD_INT, .num = 17},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(42));
	
	// if (false) 42 else 17
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_FALSE},
		(instruction_t){BC_JUMP_IF_FALSE, .jump_offset = 2},
		(instruction_t){BC_LOAD_INT, .num = 42},
		(instruction_t){BC_JUMP, .jump_offset = 1},
		(instruction_t){BC_LOAD_INT, .num = 17},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(17));
}

void pack_and_extract_test(){
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 0},
		(instruction_t){BC_LOAD_INT, .num = 8},
		(instruction_t){BC_LOAD_INT, .num = 15},
		(instruction_t){BC_PACK, .count = 3},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, array_atom(3, (atom_t[]){ int_atom(0), int_atom(8), int_atom(15) }));
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 0},
		(instruction_t){BC_LOAD_INT, .num = 8},
		(instruction_t){BC_LOAD_INT, .num = 15},
		(instruction_t){BC_PACK, .count = 3},
		(instruction_t){BC_EXTRACT, .count = 1},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(8));
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 0},
		(instruction_t){BC_LOAD_INT, .num = 8},
		(instruction_t){BC_LOAD_INT, .num = 15},
		(instruction_t){BC_PACK, .count = 3},
		(instruction_t){BC_EXTRACT, .count = 2},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(15));
}

void integer_arithmetic_test(){
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 3},
		(instruction_t){BC_LOAD_INT, .num = 4},
		(instruction_t){BC_ADD, .operand = BC_OP_NONE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(7));
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 3},
		(instruction_t){BC_LOAD_INT, .num = 4},
		(instruction_t){BC_SUB, .operand = BC_OP_NONE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(-1));
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 3},
		(instruction_t){BC_LOAD_INT, .num = 4},
		(instruction_t){BC_MUL, .operand = BC_OP_NONE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(12));
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 4},
		(instruction_t){BC_LOAD_INT, .num = 2},
		(instruction_t){BC_DIV, .operand = BC_OP_NONE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(2));
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 10},
		(instruction_t){BC_LOAD_INT, .num = 3},
		(instruction_t){BC_DIV, .operand = BC_OP_NONE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(3));
}

void float_arithmetic_test(){
	func_t func;
	module_t mod = (module_t){
		.literals = (atom_t[]){
			float_atom(1.5),
			float_atom(3.5)
		},
		.literal_count = 2,
		.functions = &func,
		.function_count = 1
	};
	
	func = sample_func((instruction_t[]){
		(instruction_t){BC_LOAD_LITERAL, .index = 0},
		(instruction_t){BC_LOAD_LITERAL, .index = 1},
		(instruction_t){BC_ADD},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	});
	check_mod_exec(&mod, float_atom(5.0));
	
	func = sample_func((instruction_t[]){
		(instruction_t){BC_LOAD_LITERAL, .index = 0},
		(instruction_t){BC_LOAD_LITERAL, .index = 1},
		(instruction_t){BC_SUB},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	});
	check_mod_exec(&mod, float_atom(-2.0));
	
	func = sample_func((instruction_t[]){
		(instruction_t){BC_LOAD_LITERAL, .index = 0},
		(instruction_t){BC_LOAD_LITERAL, .index = 1},
		(instruction_t){BC_MUL},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	});
	check_mod_exec(&mod, float_atom(5.25));
	
	func = sample_func((instruction_t[]){
		(instruction_t){BC_LOAD_LITERAL, .index = 0},
		(instruction_t){BC_LOAD_LITERAL, .index = 1},
		(instruction_t){BC_DIV},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	});
	mod.literals[0] = float_atom(10.0);
	mod.literals[1] = float_atom(4.0);
	check_mod_exec(&mod, float_atom(2.5));
}

void embeded_arithmetic_operand_test(){
	// This ADD instruction is our increment
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 5},
		(instruction_t){BC_ADD, .operand = BC_OP_RIGHT, .num = 1},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(6));
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 5},
		(instruction_t){BC_SUB, .operand = BC_OP_RIGHT, .num = 3},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(2));
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 5},
		(instruction_t){BC_SUB, .operand = BC_OP_LEFT, .num = 3},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, int_atom(-2));
}

void equals_test(){
	// The exact equality semantics are already covered by the atoms_equal_test in the common_test
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_NIL},
		(instruction_t){BC_LOAD_NIL},
		(instruction_t){BC_EQ},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, true_atom());
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_NIL},
		(instruction_t){BC_LOAD_FALSE},
		(instruction_t){BC_EQ},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, false_atom());
}

void integer_comparators_test(){
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 3},
		(instruction_t){BC_LOAD_INT, .num = 5},
		(instruction_t){BC_LT},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, true_atom());
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 5},
		(instruction_t){BC_LOAD_INT, .num = 3},
		(instruction_t){BC_LT},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, false_atom());
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 3},
		(instruction_t){BC_LOAD_INT, .num = 5},
		(instruction_t){BC_GT},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, false_atom());
	
	check_simple_func_exec((instruction_t[]){
		(instruction_t){BC_LOAD_INT, .num = 5},
		(instruction_t){BC_LOAD_INT, .num = 3},
		(instruction_t){BC_GT},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	}, true_atom());
}

void float_comparators_test(){
	func_t func;
	module_t mod = (module_t){
		.literals = (atom_t[]){
			float_atom(1.5),
			float_atom(3.5)
		},
		.literal_count = 2,
		.functions = &func,
		.function_count = 1
	};
	
	func = sample_func((instruction_t[]){
		(instruction_t){BC_LOAD_LITERAL, .index = 0},
		(instruction_t){BC_LOAD_LITERAL, .index = 1},
		(instruction_t){BC_LT},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	});
	check_mod_exec(&mod, true_atom());
	
	func = sample_func((instruction_t[]){
		(instruction_t){BC_LOAD_LITERAL, .index = 1},
		(instruction_t){BC_LOAD_LITERAL, .index = 0},
		(instruction_t){BC_LT},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	});
	check_mod_exec(&mod, false_atom());
	
	func = sample_func((instruction_t[]){
		(instruction_t){BC_LOAD_LITERAL, .index = 0},
		(instruction_t){BC_LOAD_LITERAL, .index = 1},
		(instruction_t){BC_GT},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	});
	check_mod_exec(&mod, false_atom());
	
	func = sample_func((instruction_t[]){
		(instruction_t){BC_LOAD_LITERAL, .index = 1},
		(instruction_t){BC_LOAD_LITERAL, .index = 0},
		(instruction_t){BC_GT},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_EOL}
	});
	check_mod_exec(&mod, true_atom());
}

int main(){
	interp = interpreter_new(4 * 1024);
	
	run(direct_load_instruction_test);
	run(drop_instruction_test);
	run(load_literal_test);
	run(jump_instruction_for_if_test);
	run(pack_and_extract_test);
	run(integer_arithmetic_test);
	run(float_arithmetic_test);
	run(embeded_arithmetic_operand_test);
	run(equals_test);
	run(integer_comparators_test);
	run(float_comparators_test);
	
	interpreter_destroy(interp);
	return show_report();
}