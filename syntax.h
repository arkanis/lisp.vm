#pragma once

#include <stdio.h>
#include "memory.h"

lvm_atom_p lvm_read(FILE* input, FILE* errors, lvm_p lvm);
void lvm_print(lvm_atom_p atom, FILE* output);