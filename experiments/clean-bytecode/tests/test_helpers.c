#include <string.h>

#include "test_utils.h"
#include "test_helpers.h"
#include "../common.h"

bool check_atom_func(atom_t actual, atom_t expected, module_p module, const char *file, const int line, const char *func_name, const char *code){
	char actual_inspect[1024], expected_inspect[1024];
	
	inspect_atom(actual, actual_inspect, 1024, module);
	inspect_atom(expected, expected_inspect, 1024, module);
	
	return check(atoms_equal(actual, expected), "atom check failed, got %s, expected %s", actual_inspect, expected_inspect);
}