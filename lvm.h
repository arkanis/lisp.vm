#pragma once

#include "memory.h"

void lvm_new(lvm_p lvm);
void lvm_destroy(lvm_p lvm);


/*
interpreter
	new(...)  // probably an options struct with stack size, heap size, etc.
	destroy()
	
	run(module)

compiler
	new(...)
	destroy()
	
	compile(source_code, existing_module) → module

module
	new(...)
	
	save(filename)
	load(filename)
*/