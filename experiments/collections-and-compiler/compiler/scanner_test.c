#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "../testing.h"
#include "scanner.h"


void test_scan_until(){
	int fd = open("scanner_test_code", O_RDONLY);
	check_msg(fd != -1, "failed to open the scanner_test_code file");
	scanner_p scan = scan_open(fd);
	slice_t slice;
	
	int c = scan_until(scan, &slice, '"');
	check_msg(c == '"', "expected terminator \" but got %c (%d)", c, c);
	check_msg(slice.length == 0, "expected length of 0 but got %d", slice.length);
	check_str(slice.ptr, "");
	free(slice.ptr);
	
	c = scan_until(scan, &slice, '"');
	check_msg(c == '"', "expected terminator \" but got %c (%d)", c, c);
	check_msg(slice.length == 11, "expected length of 11 but got %d", slice.length);
	check_str(slice.ptr, "hello world");
	free(slice.ptr);
	
	c = scan_until(scan, &slice, EOF);
	check_msg(c == EOF, "expected terminator EOF but got %d", c);
	check_str(slice.ptr, "\n\t \n1234567890\n----");
	free(slice.ptr);
	
	scan_close(scan);
	close(fd);
}

void test_scan_while(){
	int fd = open("scanner_test_code", O_RDONLY);
	check_msg(fd != -1, "failed to open the scanner_test_code file");
	scanner_p scan = scan_open(fd);
	slice_t slice;
	
	// Throw the first two lines away
	scan_until(scan, &slice, '\n');
	check_str(slice.ptr, "\"hello world\"");
	free(slice.ptr);
	scan_until(scan, &slice, '\n');
	check_str(slice.ptr, "\t ");
	free(slice.ptr);
	
	int c = scan_while(scan, &slice, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9');
	check_msg(c == '\n', "expected newline as terminator but got %d", c);
	check_msg(slice.length == 10, "expected length of 10 but got %d", slice.length);
	check_str(slice.ptr, "1234567890");
	free(slice.ptr);
	
	c = scan_while(scan, &slice, '\n', '-');
	check_msg(c == EOF, "expected EOF as terminator but got %d", c);
	check_msg(slice.length == 5, "expected length of 10 but got %d", slice.length);
	check_str(slice.ptr, "\n----");
	free(slice.ptr);
	
	scan_close(scan);
	close(fd);
}

void test_scan_one_of(){
	int fd = open("scanner_test_code", O_RDONLY);
	check_msg(fd != -1, "failed to open the scanner_test_code file");
	scanner_p scan = scan_open(fd);
	
	int c = scan_one_of(scan, '"');
	check_msg(c == '"', "expected an \", got %c", c);
	
	// This tests if scan_one_of really consumes one character and not only
	// increments the buffer position.
	slice_t slice;
	c = scan_until(scan, &slice, '"');
	check_str(slice.ptr, "hello world");
	free(slice.ptr);
	
	c = scan_one_of(scan, '\n');
	check_msg(c == '\n', "expected a line break, got %d", c);
	c = scan_one_of(scan, ' ', '\n', '\t');
	check_msg(c == '\t', "expected a tab, got %d", c);
	
	scan_close(scan);
	close(fd);
}

void test_scan_until_func(){
	int fd = open("scanner_test_code", O_RDONLY);
	check_msg(fd != -1, "failed to open the scanner_test_code file");
	scanner_p scan = scan_open(fd);
	slice_t slice;
	
	int c = scan_until_func(scan, &slice, isspace);
	check_msg(c == ' ', "expected a space as terminator but got %d", c);
	check_msg(slice.length == 6, "expected length of 6 but got %d", slice.length);
	check_str(slice.ptr, "\"hello");
	free(slice.ptr);
	
	scan_close(scan);
	close(fd);
}

void test_scan_while_func(){
	int fd = open("scanner_test_code", O_RDONLY);
	check_msg(fd != -1, "failed to open the scanner_test_code file");
	scanner_p scan = scan_open(fd);
	slice_t slice;
	
	int c = scan_while_func(scan, &slice, ispunct, islower, isblank);
	check_msg(c == '\n', "expected newline as terminator but got %c", c);
	check_msg(slice.length == 13, "expected length of 13 but got %d", slice.length);
	check_str(slice.ptr, "\"hello world\"");
	free(slice.ptr);
	
	scan_close(scan);
	close(fd);
}

void test_line_and_col_numbers(){
	int fd = open("scanner_test_code", O_RDONLY);
	check_msg(fd != -1, "failed to open the scanner_test_code file");
	scanner_p scan = scan_open(fd);
	
	check_msg(scan->line == 1, "the line number should start with 1 but got %d", scan->line);
	check_msg(scan->col == 1, "the column number should start with 1 but got %d", scan->col);
	
	int c = scan_until(scan, NULL, '"');
	check_msg(scan->line == 1, "the line number should not change but it did: %d", scan->line);
	check_msg(scan->col == 2, "one char consumed, col should be 2 but is %d", scan->col);
	
	c = scan_until(scan, NULL, '"');
	check_msg(scan->line == 1, "the line number should not change but it did: %d", scan->line);
	check_msg(scan->col == 14, "we're at the end of the first line, col is supposed to be 14 but is %d", scan->col);
	
	c = scan_while_func(scan, NULL, isspace);
	check_msg(c == '1', "expected 1 as terminator but got %c", c);
	check_msg(scan->line == 3, "the line number should not change but it did: %d", scan->line);
	check_msg(scan->col == 1, "we're at the start of the third line, col is supposed to be 1 but is %d", scan->col);
	
	scan_close(scan);
	close(fd);
}

void test_string_scanner(){
	scanner_p scan = scan_open_string("say(hello)");
	slice_t slice;
	
	int c = scan_while_func(scan, &slice, isalpha);
	check_msg(c == '(', "expected ( as terminator but got %c", c);
	check_msg(slice.length == 3, "expected 3 characters for the first word but got length %d", slice.length);
	check_str(slice.ptr, "say");
	free(slice.ptr);
	
	c = scan_one_of(scan, '(');
	check_msg(c == '(', "somehow reading the opening braces failed, got %c", c);
	
	c = scan_until(scan, &slice, ')');
	check_msg(c == ')', "expected ) as terminator but got %c", c);
	check_str(slice.ptr, "hello");
	free(slice.ptr);
	
	c = scan_until(scan, &slice, EOF);
	check_msg(c == EOF, "expected EOF as terminator but got %d", c);
	check_msg(slice.length == 0, "expected an empty string until EOF but got length %d", slice.length);
	check_str(slice.ptr, "");
	free(slice.ptr);
	
	scan_close(scan);
}

void test_scan_peek(){
	scanner_p scan = scan_open_string("say(hello)");
	
	int c = scan_peek(scan);
	check_msg(c == 's', "expected the first character but got %c", c);
	check_msg(scan->col == 1, "expected to still be at column 1 but got column %d", scan->col);
	
	slice_t slice;
	c = scan_while_func(scan, NULL, isalpha);
	check_msg(c == '(', "expected ( as terminator but got %c", c);
	check_str(slice.ptr, "say");
	free(slice.ptr);
	
	c = scan_peek(scan);
	check_msg(c == '(', "expected ( but got %c", c);
	
	c = scan_until(scan, NULL, EOF);
	check_msg(c == EOF, "expected EOF as terminator but got %d", c);
	
	c = scan_peek(scan);
	check_msg(c == EOF, "expected EOF but got %d", c);
	
	scan_close(scan);
}



int main(){
	run(test_scan_until);
	run(test_scan_while);
	run(test_scan_one_of);
	run(test_scan_until_func);
	run(test_scan_while_func);
	run(test_line_and_col_numbers);
	run(test_string_scanner);
	
	return show_report();
}