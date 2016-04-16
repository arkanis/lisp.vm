#pragma once

#include <stdint.h>
#include <stdbool.h>

//
// Bytecode instruction structure and constants
//

typedef struct {
	uint8_t op;
	
	union {
		uint8_t scope_offset;
		uint8_t operand;
	};
	union {
		int16_t num;
		uint16_t index;
		uint16_t count;
		int16_t jump_offset;
	};
} instruction_t, *instruction_p;


#define BC_EOL			0	// no real instruction, just end of list marker in instruction lists

#define BC_LOAD_NIL		1
#define BC_LOAD_TRUE		2
#define BC_LOAD_FALSE		3
#define BC_LOAD_INT		4	// num
#define BC_LOAD_LITERAL	5	// index
#define BC_LOAD_FUNC		6	// index

#define BC_LOAD_ARG		7	// scope_offset, index
#define BC_LOAD_LOCAL		8	// scope_offset, index
#define BC_STORE_LOCAL	9	// scope_offset, index

#define BC_DROP			10	// count
#define BC_CALL			11	// count
#define BC_RETURN			12	// count?

#define BC_JUMP			13	// jump_offset
#define BC_JUMP_IF_FALSE	14	// jump_offset

#define BC_PACK			15	// count
#define BC_EXTRACT		16	// index

// Instructions with embedable second operator. With "a op b" either a or b can be embedded into
// the instruction. The `operand` field specifies which operator is embeded (left, right or none), the
// `num` field holds the value.
#define BC_OP_NONE		0
#define BC_OP_LEFT			1
#define BC_OP_RIGHT		2

#define BC_ADD			17
#define BC_SUB			18
#define BC_MUL			19
#define BC_DIV			20

#define BC_EQ				21
#define BC_LT				22
#define BC_GT				23


#define BC_AND			24
#define BC_OR				25
#define BC_NOT			26




//
// Memory structures
//

typedef struct atom atom_t, *atom_p;

struct atom {
	// header word
	uint8_t type;
	uint8_t padding1, padding2, padding3;
	
	union {
		uint32_t len;
		uint32_t index;
	};
	
	// content word
	union {
		int64_t inum;
		double fnum;
		char *str;
		atom_p atoms;
		atom_p parent_frame;
	};
};


#define T_NIL		1
#define T_TRUE	2
#define T_FALSE	3
#define T_INT		4	// inum
#define T_FLOAT	5	// fnum
#define T_SYM		6	// index (sym table index)
#define T_STR		7	// len, str pointer
#define T_ARRAY	8	// len, atoms pointer
#define T_FUNC	9	// index (func table index), parent_frame

static inline atom_t nil_atom(){ return (atom_t){T_NIL}; }
static inline atom_t true_atom(){ return (atom_t){T_TRUE}; }
static inline atom_t false_atom(){ return (atom_t){T_FALSE}; }

static inline atom_t int_atom(int64_t val){ return (atom_t){T_INT, .inum = val}; }
static inline atom_t float_atom(double val){ return (atom_t){T_FLOAT, .fnum = val}; }
static inline atom_t sym_atom(uint32_t index){ return (atom_t){T_SYM, .index = index}; }
static inline atom_t str_atom(uint32_t len, char *str){ return (atom_t){T_STR, .len = len, .str = str}; }

static inline atom_t array_atom(uint32_t len, atom_p atoms){ return (atom_t){T_ARRAY, .len = len, .atoms = atoms}; }
static inline atom_t func_atom(uint32_t index, atom_p parent_frame){ return (atom_t){T_FUNC, .index = index, .parent_frame = parent_frame}; }

bool atoms_equal(atom_t a, atom_t b);


//
// Program structures
//

typedef struct {
	size_t length;
	const char *name;
} symbol_t, *symbol_p;

typedef struct {
	instruction_p instructions;
	size_t instruction_count;
	size_t arg_count, var_count;
	
	// Compile time information
	char *names;
	
	// Inspection/debug information
	char *name;
	char *file;
	size_t line;
} func_t, *func_p;

typedef struct {
	symbol_p symbols;
	size_t symbol_count;
	atom_p literals;
	size_t literal_count;
	func_p functions;
	size_t function_count;
} module_t, *module_p;

int inspect_atom(atom_t atom, char *buffer, size_t buffer_size, module_p module);