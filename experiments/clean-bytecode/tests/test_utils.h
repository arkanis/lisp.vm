#pragma once

#include <stdio.h>
#include <stdbool.h>

#define check(expr, ...) check_func( (expr), __FILE__, __LINE__, __func__, #expr, __VA_ARGS__)
#define check_str(actual, expected) check_str_func( (actual), (expected), __FILE__, __LINE__, __func__, #actual " == " #expected)
#define check_int(actual, expected) check_int_func( (actual), (expected), __FILE__, __LINE__, __func__, #actual " == " #expected)

typedef void (*test_func_t)();

bool run(test_func_t test);
bool check_func(bool expr, const char *file, const int line, const char *func_name, const char *code, const char *message, ...);
bool check_str_func(const char *actual, const char *expected, const char *file, const int line, const char *func_name, const char *code);
bool check_int_func(int actual, int expected, const char *file, const int line, const char *func_name, const char *code);
int show_report();