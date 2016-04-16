#pragma once

#include "common.h"


typedef struct {
	atom_p stack;
	size_t stack_size;
} interpreter_t, *interpreter_p;


interpreter_p interpreter_new(size_t stack_size);
void interpreter_destroy(interpreter_p interpreter);

atom_t interpreter_exec(interpreter_p interpreter, module_p module);