#pragma once

#include "../common.h"

#define check_atom(actual, expected, module) check_atom_func( (actual), (expected), (module), __FILE__, __LINE__, __func__, #actual " == " #expected)

bool check_atom_func(atom_t actual, atom_t expected, module_p module, const char *file, const int line, const char *func_name, const char *code);