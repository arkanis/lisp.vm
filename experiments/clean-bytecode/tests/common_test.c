#include <string.h>

#include "test_utils.h"
#include "../common.h"

void atoms_equal_test(){
	check(atoms_equal(nil_atom(), nil_atom()) == true, "");
	check(atoms_equal(true_atom(), false_atom()) == false, "");
	
	check(atoms_equal(int_atom(7), int_atom(7)) == true, "");
	check(atoms_equal(int_atom(7), int_atom(-8)) == false, "");
	
	check(atoms_equal(float_atom(7.5), float_atom(7.5)) == true, "");
	check(atoms_equal(float_atom(7.5), float_atom(-8.5)) == false, "");
	
	check(atoms_equal(sym_atom(1), sym_atom(1)) == true, "");
	check(atoms_equal(sym_atom(1), sym_atom(2)) == false, "");
	
	check(atoms_equal(str_atom(5, "hallo"), str_atom(5, "hallo")) == true, "");
	check(atoms_equal(str_atom(5, "hallo"), str_atom(3, "you")) == false, "");
	check(atoms_equal(str_atom(5, "hallo"), str_atom(5, "world")) == false, "");
	
	check(atoms_equal(
		array_atom(3, (atom_t[]){ nil_atom(), int_atom(42), true_atom() }),
		array_atom(3, (atom_t[]){ nil_atom(), int_atom(42), true_atom() })
	) == true, "");
	check(atoms_equal(
		array_atom(3, (atom_t[]){ nil_atom(), int_atom(42), true_atom() }),
		array_atom(2, (atom_t[]){ int_atom(42), true_atom() })
	) == false, "");
	check(atoms_equal(
		array_atom(3, (atom_t[]){ nil_atom(), int_atom(42), true_atom() }),
		array_atom(3, (atom_t[]){ nil_atom(), int_atom(42), false_atom() })
	) == false, "");
	
	check(atoms_equal(func_atom(0, (atom_p) 0x400e3d), func_atom(0, (atom_p) 0x400e3d)) == true, "");
	check(atoms_equal(func_atom(0, (atom_p) 0x400e3d), func_atom(3, NULL)) == false, "");
	check(atoms_equal(func_atom(0, (atom_p) 0x400e3d), func_atom(0, NULL)) == false, "");
}

void inspect_test_without_module(){
	size_t buffer_size = 1024, bytes_written = 0;
	char buffer[buffer_size];
	
	bytes_written = inspect_atom(nil_atom(), buffer, buffer_size, NULL);
	check_str(buffer, "nil");
	check_int(strlen(buffer), bytes_written);
	
	bytes_written = inspect_atom(true_atom(), buffer, buffer_size, NULL);
	check_str(buffer, "true");
	check_int(strlen(buffer), bytes_written);
	
	bytes_written = inspect_atom(false_atom(), buffer, buffer_size, NULL);
	check_str(buffer, "false");
	check_int(strlen(buffer), bytes_written);
	
	bytes_written = inspect_atom(int_atom(42), buffer, buffer_size, NULL);
	check_str(buffer, "42");
	check_int(strlen(buffer), bytes_written);
	
	bytes_written = inspect_atom(float_atom(0.5), buffer, buffer_size, NULL);
	check_str(buffer, "0.500000");
	check_int(strlen(buffer), bytes_written);
	
	bytes_written = inspect_atom(sym_atom(7), buffer, buffer_size, NULL);
	check_str(buffer, "symbol(7)");
	check_int(strlen(buffer), bytes_written);
	
	bytes_written = inspect_atom(str_atom(3, "foo"), buffer, buffer_size, NULL);
	check_str(buffer, "\"foo\"");
	check_int(strlen(buffer), bytes_written);
	
	bytes_written = inspect_atom(array_atom(3, (atom_t[]){ nil_atom(), int_atom(42), true_atom() }), buffer, buffer_size, NULL);
	check_str(buffer, "[nil, 42, true]");
	check_int(strlen(buffer), bytes_written);
	
	bytes_written = inspect_atom(func_atom(7, (atom_p) 0x400e3d), buffer, buffer_size, NULL);
	check_str(buffer, "func(7, 0x400e3d)");
	check_int(strlen(buffer), bytes_written);
}

int main(){
	run(atoms_equal_test);
	run(inspect_test_without_module);
	return show_report();
}