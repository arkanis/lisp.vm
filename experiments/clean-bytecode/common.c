#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "common.h"

bool atoms_equal(atom_t a, atom_t b){
	if (a.type != b.type)
		return false;
	
	switch(a.type){
		case T_NIL: case T_TRUE: case T_FALSE:
			return true;
		case T_INT:  // inum
			return a.inum == b.inum;
		case T_FLOAT:  // fnum
			return a.fnum == b.fnum;
		case T_SYM:  // index (sym table index)
			return a.index == b.index;
		case T_STR:  // len, str pointer
			return a.len == b.len && strncmp(a.str, b.str, a.len) == 0;
		case T_ARRAY:  // len, atoms pointer
			if (a.len != b.len)
				return false;
			for(size_t i = 0; i < a.len; i++){
				if ( ! atoms_equal(a.atoms[i], b.atoms[i]) )
					return false;
			}
			return true;
		case T_FUNC:  // index (func table index), parent_frame
			return a.index == b.index && a.parent_frame == b.parent_frame;
	}
	
	return true;
}

int inspect_atom(atom_t atom, char *buffer, size_t buffer_size, module_p module){
	switch(atom.type){
		case T_NIL:
			return snprintf(buffer, buffer_size, "nil");
		case T_TRUE:
			return snprintf(buffer, buffer_size, "true");
		case T_FALSE:
			return snprintf(buffer, buffer_size, "false");
		case T_INT:
			return snprintf(buffer, buffer_size, "%ld", atom.inum);
		case T_FLOAT:
			return snprintf(buffer, buffer_size, "%lf", atom.fnum);
		case T_SYM:
			if (module == NULL)
				return snprintf(buffer, buffer_size, "symbol(%d)", atom.index);
			
			assert(atom.index < module->symbol_count);
			return snprintf(buffer, buffer_size, "%*s", (int)module->symbols[atom.index].length, module->symbols[atom.index].name);
		case T_STR:
			return snprintf(buffer, buffer_size, "\"%*s\"", atom.len, atom.str);
		case T_ARRAY: {
			size_t buffer_filled = 0;
			int ret = 0;
			ret = snprintf(buffer + buffer_filled, buffer_size - buffer_filled, "[");
			if (ret < 0) return ret;
			buffer_filled += ret;
			
			// Inspect all members recursively
			for(size_t i = 0; i < atom.len; i++){
				ret = inspect_atom(atom.atoms[i], buffer + buffer_filled, buffer_size - buffer_filled, module);
				if (ret < 0) return ret;
				buffer_filled += ret;
				
				// Skip the delimiter for the last atom
				if (i + 1 == atom.len)
					continue;
				
				ret = snprintf(buffer + buffer_filled, buffer_size - buffer_filled, ", ");
				if (ret < 0) return ret;
				buffer_filled += ret;
			}
			
			ret = snprintf(buffer + buffer_filled, buffer_size - buffer_filled, "]");
			if (ret < 0) return ret;
			buffer_filled += ret;
			
			return buffer_filled;
			} break;
		case T_FUNC: {
			if (module == NULL)
				return snprintf(buffer, buffer_size, "func(%d, %p)", atom.index, atom.parent_frame);
			
			assert(atom.index < module->function_count);
			func_p func = module->functions + atom.index;
			
			size_t buffer_filled = 0;
			int ret = 0;
			ret = snprintf(buffer + buffer_filled, buffer_size - buffer_filled, "%s(", func->name);
			if (ret < 0) return ret;
			buffer_filled += ret;
			
			// Add all argument names
			for(size_t i = 0; i < func->arg_count; i++){
				ret = snprintf(buffer + buffer_filled, buffer_size - buffer_filled, "%s", func->names + i);
				if (ret < 0) return ret;
				buffer_filled += ret;
				
				// Skip the delimiter for the last arg name
				if (i + 1 == func->arg_count)
					continue;
				
				ret = snprintf(buffer + buffer_filled, buffer_size - buffer_filled, ", ");
				if (ret < 0) return ret;
				buffer_filled += ret;
			}
			
			return buffer_filled;
			} break;
	}
	
	return snprintf(buffer, buffer_size, "unknown(type %d)", atom.type);
}