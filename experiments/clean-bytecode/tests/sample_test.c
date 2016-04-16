#include "test_utils.h"

void fail_test(){
	check(1 == 0, "supposed to fail");
}

void abort_test(){
	check(1 == 1, NULL);
	check(1 == 0, NULL);
	check(1 == 0, "should never be reached");
}

void success_test(){
	check(1 == 1, NULL);
	check(2 == 2, NULL);
}

void int_test(){
	check_int(7, 7);
	check_int(7, 42);
}

void str_test(){
	check_str("foo", "foo");
	check_str("foo", "bar");
}

int main(){
	run(fail_test);
	run(abort_test);
	run(success_test);
	run(int_test);
	run(str_test);
	return show_report();
}