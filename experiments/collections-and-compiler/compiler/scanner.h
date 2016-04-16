#pragma once

#include <stdbool.h>

typedef struct {
	int fd;
	char *buffer_ptr;
	size_t buffer_size, buffer_pos, buffer_consumed, buffer_filled;
	size_t line, col, prev_col;
	bool eof;
} scanner_t, *scanner_p;

typedef struct {
	char *ptr;
	size_t length;
} slice_t, *slice_p;

typedef int (*scanner_check_func_t)(int);

scanner_p scan_open(int fd);
scanner_p scan_open_string(char *code);
void scan_close(scanner_p scanner);

int scan_peek(scanner_p scanner);

#define scan_until(scanner, slice, ...)       scan_until_with_raw_args(scanner, slice, (int[]){ __VA_ARGS__, -2 })
#define scan_while(scanner, slice, ...)       scan_while_with_raw_args(scanner, slice, (int[]){ __VA_ARGS__, -2 })
#define scan_until_func(scanner, slice, ...)  scan_until_func_with_raw_args(scanner, slice, (scanner_check_func_t[]){ __VA_ARGS__, NULL })
#define scan_while_func(scanner, slice, ...)  scan_while_func_with_raw_args(scanner, slice, (scanner_check_func_t[]){ __VA_ARGS__, NULL })
#define scan_one_of(scanner, ...)             scan_one_of_with_raw_args(scanner, (int[]){ __VA_ARGS__, -2 })

int scan_until_with_raw_args(scanner_p scanner, slice_t* slice, int tokens[]);
int scan_while_with_raw_args(scanner_p scanner, slice_t* slice, int tokens[]);
int scan_until_func_with_raw_args(scanner_p scanner, slice_t* slice, scanner_check_func_t funcs[]);
int scan_while_func_with_raw_args(scanner_p scanner, slice_t* slice, scanner_check_func_t funcs[]);
int scan_one_of_with_raw_args(scanner_p scanner, int tokens[]);