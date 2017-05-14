/**

Builds on the previous AST interpreter and adds bits and pieces for a garbage collector and an atom stack. The stack is
meant to hold all the references to atoms so the GC can scan and patch them in one place.

Abandoned becase it meant to rewrite the entire interpreter to use stack indices instead of direct atom pointers.

- The code is ugly as hell.
- Not covered by any test cases.

**/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include <sys/mman.h>


typedef enum {
	T_NIL,
	T_TRUE,
	T_FALSE,
	T_SYM,
	T_NUM,
	T_STR,
	T_ARRAY,
	T_OBJ,
	T_ENV,
	T_ERROR,
	T_BUILTIN,
	T_SYNTAX,
	T_LAMBDA
} atom_type_t;

typedef struct atom_s atom_t, *atom_p;

typedef struct {
	uint32_t hash;
	const char* key;  // 0: slot is free, 1: slot is deleted, rest: pointer to key string
	atom_p value;
} obj_slot_t, *obj_slot_p;

#define OBJ_SLOT_FREE    (const char*)0
#define OBJ_SLOT_DELETED (const char*)1

typedef atom_p (*eval_proc_t)(atom_p args, atom_p env);

struct atom_s {
	atom_type_t type;
	union {
		double num;
		char* str;
		char* sym;
		struct { size_t len; atom_p* ptr; } array;
		struct {
			atom_p parent;
			uint32_t len, cap, deleted;
			obj_slot_p slots;
		} obj;
		char* error;
		eval_proc_t builtin;
		eval_proc_t syntax;
		struct {
			atom_p args;
			atom_p body;
			atom_p env;
		} lambda;
	};
};

atom_p nil_atom = NULL;
atom_p true_atom = NULL;
atom_p false_atom = NULL;
atom_p base_env = NULL;


//
// Memory management
//

typedef struct {
	void* ptr;
	size_t size;
	size_t filled;
} space_t, *space_p;

space_t spaces[2] = { { 0 }, { 0 } };
space_p new_space = &spaces[0], old_space = &spaces[1];

typedef size_t atom_id_t;
atom_p stack = NULL;
size_t stack_size = 0, sp = 0;


void gc_init() {
	new_space->size = 16*1024*1024;
	new_space->ptr = mmap(NULL, new_space->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	new_space->filled = 0;
	
	stack_size = 1*1024*1024;
	stack = mmap(NULL, stack_size*sizeof(stack[0]), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
}

atom_id_t stack_push(atom_p atom) {
	sp++;
	
	if (sp > stack_size) {
		size_t new_stack_size = stack_size * 2;
		stack = mremap(stack, stack_size*sizeof(stack[0]), new_stack_size*sizeof(stack[0]), MREMAP_MAYMOVE);
	}
	
	return sp - 1;
}

void stack_drop() {
	sp--;
}

atom_p a(atom_id_t id) {
	return stack[id];
}

atom_id_t gc_alloc_atom(atom_type_t type) {
	atom_p atom = gc_alloc_data(sizeof(atom_t));
	atom->type = type;
	return stack_push(atom);
}

void* gc_alloc_data(size_t size) {
	if (new_space->filled + size > new_space->size) {
		// not enough space, we need to collect
		// TODO: collect
	}
	
	void* ptr = new_space->ptr + new_space->filled;
	new_space->filled += size;
	return ptr;
}

atom_id_t sym_atom(char* symbol) {
	atom_id_t id = gc_alloc_atom(T_SYM);
	void* data = gc_alloc_data(strlen(symbol));
	strcpy(data, symbol);
	a(id)->sym = data;
	return id;
}

atom_id_t str_atom(char* string) {
	atom_id_t id = gc_alloc_atom(T_STR);
	void* data = gc_alloc_data(strlen(string));
	strcpy(data, string);
	a(id)->str = data;
	return id;
}

atom_id_t num_atom(double value) {
	atom_id_t id = gc_alloc_atom(T_NUM);
	a(id)->num = value;
	return id;
}

atom_id_t array_atom() {
	atom_id_t id = gc_alloc_atom(T_ARRAY);
	a(id)->array.len = 0;
	a(id)->array.ptr = NULL;
	return id;
}




atom_p error_atom(char* message) {
	atom_p atom = malloc(sizeof(atom_t));
	atom->type = T_ERROR;
	atom->error = message;
	return atom;
}

atom_p syntax_atom(eval_proc_t func) {
	atom_p atom = malloc(sizeof(atom_t));
	atom->type = T_SYNTAX;
	atom->builtin = func;
	return atom;
}

atom_p builtin_atom(eval_proc_t func) {
	atom_p atom = malloc(sizeof(atom_t));
	atom->type = T_BUILTIN;
	atom->builtin = func;
	return atom;
}


//
// Hash table for objects and environments
//

obj_slot_p hash_next(atom_p obj, obj_slot_p it);
void hash_set(atom_p obj, const char* key, atom_p value);


obj_slot_p hash_start(atom_p obj) {
	// We need to start at an invalid slot address since hash_next() increments it
	// before it looks at it (so it's safe).
	return hash_next(obj, obj->obj.slots - 1);
}

obj_slot_p hash_next(atom_p obj, obj_slot_p it) {
	assert(obj->type == T_OBJ || obj->type == T_ENV);
	if (it == NULL)
		return NULL;
	
	do {
		it++;
		// Check if we're past the last slot
		if (it - obj->obj.slots >= obj->obj.cap)
			return NULL;
	} while( it->key == OBJ_SLOT_FREE || it->key == OBJ_SLOT_DELETED );
	
	return it;
}

void hash_resize(atom_p obj, uint32_t new_capacity) {
	assert(obj->type == T_OBJ || obj->type == T_ENV);
	
	atom_t new_obj;
	new_obj.type = obj->type;
	new_obj.obj.parent = obj->obj.parent;
	new_obj.obj.len = 0;
	new_obj.obj.cap = new_capacity;
	new_obj.obj.deleted = 0;
	new_obj.obj.slots = calloc(new_capacity, sizeof(new_obj.obj.slots[0]));
	
	for(obj_slot_p it = hash_start(obj); it != NULL; it = hash_next(obj, it)) {
		hash_set(&new_obj, it->key, it->value);
	}
	
	free(obj->obj.slots);
	*obj = new_obj;
}

uint32_t fnv1a(const char* key) {
	uint32_t hash = 2166136261;
	for(uint8_t c = *key; c != '\0'; c++) {
		hash ^= c;
		hash *= 16777619;
	}
	return hash;
}

atom_p hash_get(atom_p obj, const char* key) {
	assert(obj->type == T_OBJ || obj->type == T_ENV);
	
	uint32_t hash = fnv1a(key);
	size_t index = hash % obj->obj.cap;
	
	while( !(obj->obj.slots[index].key == OBJ_SLOT_FREE) ) {
		if ( obj->obj.slots[index].hash == hash && strcmp(obj->obj.slots[index].key, key) == 0 )
			return obj->obj.slots[index].value;
		index = (index + 1) % obj->obj.cap;
	}
	
	return NULL;
}

void hash_set(atom_p obj, const char* key, atom_p value) {
	assert(obj->type == T_OBJ || obj->type == T_ENV);
	
	if (obj->obj.len + obj->obj.deleted + 1 > obj->obj.cap * 0.7)
		hash_resize(obj, (obj->obj.cap == 0) ? 2 : obj->obj.cap * 2);
	
	uint32_t hash = fnv1a(key);
	size_t index = hash % obj->obj.cap;
	
	while ( !(obj->obj.slots[index].key == OBJ_SLOT_FREE || obj->obj.slots[index].key == OBJ_SLOT_DELETED) ) {
		if ( obj->obj.slots[index].hash == hash && strcmp(obj->obj.slots[index].key, key) == 0 )
			break;
		index = (index + 1) % obj->obj.cap;
	}
	
	if (obj->obj.slots[index].key == OBJ_SLOT_DELETED)
		obj->obj.deleted--;
	obj->obj.len++;
	obj->obj.slots[index].hash = hash;
	obj->obj.slots[index].key = key;
	obj->obj.slots[index].value = value;
}

bool hash_del(atom_p obj, const char* key) {
	assert(obj->type == T_OBJ || obj->type == T_ENV);
	
	uint32_t hash = fnv1a(key);
	size_t index = hash % obj->obj.cap;
	
	while ( !(obj->obj.slots[index].key == OBJ_SLOT_FREE) ) {
		if ( obj->obj.slots[index].hash == hash && strcmp(obj->obj.slots[index].key, key) == 0 ) {
			obj->obj.slots[index].hash = 0;
			obj->obj.slots[index].key = OBJ_SLOT_DELETED;
			obj->obj.slots[index].value = NULL;
			
			obj->obj.len--;
			obj->obj.deleted++;
			
			if (obj->obj.len < obj->obj.cap * 0.2)
				hash_resize(obj, obj->obj.cap / 2);
			
			return true;
		}
		
		index = (index + 1) % obj->obj.cap;
	}
	
	return false;
}


//
// Reader and printer
//
/**

EOF              → NULL
123              → number
"foo"            → string
(1 2 3)          → array
{a: 1, b: 2}     → object
nil, true, false → nil, true, false
foo              → symbol

**/

atom_p read(FILE* input) {
	fscanf(input, " ");
	if (feof(input))
		return NULL;
	
	/* Doesn't work because %lf consumes "ni" of "nil" chars. No idea why.
	double num = 0;
	if ( fscanf(input, "%lf", &num) == 1 ) {
		atom_p atom = malloc(sizeof(atom_t));
		atom->type = T_NUM;
		atom->num = num;
		return atom;
	}
	*/
	
	char* str = NULL;
	if ( fscanf(input, "\"%m[^\"]\"", &str) == 1 ) {
		atom_p atom = malloc(sizeof(atom_t));
		atom->type = T_STR;
		atom->str = str;
		return atom;
	}
	
	int c = getc(input);
	if ( isdigit(c) ) {
		double value = c - '0';
		while ( (c = getc(input)), c >= '0' && c <= '9' ) {
			value = value * 10;
			value += c - '0';
		}
		
		if (c == '.') {
			double fraction = 0.1;
			while ( (c = getc(input)), c >= '0' && c <= '9' ) {
				value += (c - '0') * fraction;
				fraction /= 10;
			}
		}
		
		ungetc(c, input);
		
		atom_p atom = malloc(sizeof(atom_t));
		atom->type = T_NUM;
		atom->num = value;
		return atom;
	} else if (c == '(') {
		atom_p atom = malloc(sizeof(atom_t));
		atom->type = T_ARRAY;
		atom->array.len = 0;
		atom->array.ptr = NULL;
		
		int c = 0;
		while (true) {
			fscanf(input, " ");
			if ( (c = getc(input)) == ')' )
				return atom;
			ungetc(c, input);
			
			atom_p item = read(input);
			if (item == NULL)
				return error_atom("unexpected EOF while reading array elements");
			
			atom->array.len += 1;
			atom->array.ptr = realloc(atom->array.ptr, atom->array.len * sizeof(atom->array.ptr[0]));
			atom->array.ptr[atom->array.len - 1] = item;
		}
	} else if (c == '{') {
		atom_p atom = calloc(1, sizeof(atom_t));
		atom->type = T_OBJ;
		
		char c = 0;
		fscanf(input, " %c", &c);
		if (c == '}')
			return atom;
		ungetc(c, input);
		
		do {
			char* key = NULL;
			if ( fscanf(input, " %m[^:{}() \f\n\r\t\v]", &key) != 1 )
				return error_atom("expected object property name after '{' or ','");
			
			fscanf(input, " %c", &c);
			if (c != ':')
				return error_atom("expected ':' after object property name");
			
			atom_p value = read(input);
			hash_set(atom, key, value);
		} while( fscanf(input, " %c", &c) == 1 && c == ',' );
		
		if (c != '}')
			return error_atom("expected '}' or ',' after object property value");
		return atom;
	} else {
		ungetc(c, input);
	}
	
	if ( fscanf(input, " %m[^:{}() \f\n\r\t\v]", &str) == 1 ) {
		atom_p atom = NULL;
		
		if ( strcmp(str, "nil") == 0 ) {
			atom = nil_atom;
			free(str);
		} else if ( strcmp(str, "true") == 0 ) {
			atom = true_atom;
			free(str);
		} else if ( strcmp(str, "false") == 0 ) {
			atom = false_atom;
			free(str);
		} else {
			atom = malloc(sizeof(atom_t));
			atom->type = T_SYM;
			atom->sym = str;
		}
		
		return atom;
	}
	
	fscanf(input, "%*[^\n]");
	return error_atom("syntax error, ignoring rest of line");
}

void print(FILE* output, atom_p atom) {
	switch(atom->type) {
		case T_NIL:
			fprintf(output, "nil");
			break;
		case T_TRUE:
			fprintf(output, "true");
			break;
		case T_FALSE:
			fprintf(output, "false");
			break;
		case T_SYM:
			fprintf(output, "%s", atom->sym);
			break;
		case T_NUM:
			fprintf(output, "%lf", atom->num);
			break;
		case T_STR:
			fprintf(output, "\"%s\"", atom->str);
			break;
		case T_ARRAY:
			fprintf(output, "(");
			for(size_t i = 0; i < atom->array.len; i++) {
				print(output, atom->array.ptr[i]);
				if (i != atom->array.len - 1)
					fprintf(output, " ");
			}
			fprintf(output, ")");
			break;
		case T_OBJ:
		case T_ENV:
			fprintf(output, "{");
			const char* sep = "";
			for(obj_slot_p it = hash_start(atom); it != NULL; it = hash_next(atom, it)) {
				fprintf(output, "%s %s: ", sep, it->key);
				print(output, it->value);
				sep = ",";
			}
			fprintf(output, " }");
			break;
		case T_ERROR:
			fprintf(output, "ERROR: %s", atom->error);
			break;
		case T_BUILTIN:
			fprintf(output, "<builtin %p>", atom->builtin);
			break;
		case T_SYNTAX:
			fprintf(output, "<syntax %p>", atom->syntax);
			break;
		case T_LAMBDA:
			fprintf(output, "<lambda ");
			print(output, atom->lambda.args);
			fprintf(output, " ");
			print(output, atom->lambda.body);
			fprintf(output, " >");
			break;
	}
}


//
// Evaluator
//
/**

nil, true, false, num, str, obj, error → self
sym → env lookup
array → func call
	eval func slot
	func slot == error
		return error
	func slot == builtin
		eval all args
		return error if args contain error
		call builtin with args
		return result
	func slot == syntax
		return error if args contain error
		call builtin with unevaled args
		return result
env → error

**/
atom_p eval(atom_p atom, atom_p env) {
	switch(atom->type) {
		case T_NIL: case T_TRUE: case T_FALSE:
		case T_NUM: case T_STR: case T_OBJ:
		case T_ERROR:
			return atom;
		
		case T_SYM:
			for(atom_p e = env; e != NULL; e = e->obj.parent) {
				atom_p value = hash_get(e, atom->sym);
				if (value)
					return value;
			}
			return nil_atom;
		
		case T_ARRAY:
			{
				if (atom->array.len == 0)
					return error_atom("eval: array needs at least one elment to be evaled");
				
				atom_p func = eval(atom->array.ptr[0], env);
				if (func->type == T_ERROR)
					return func;
				if ( !(func->type == T_BUILTIN || func->type == T_SYNTAX || func->type == T_LAMBDA) )
					return error_atom("eval: only builtin, syntax and lambda atoms are allowed in the function slot");
				
				atom_p args = malloc(sizeof(atom_t));
				args->type = T_ARRAY;
				
				if (func->type == T_BUILTIN) {
					args->array.len = atom->array.len - 1;
					args->array.ptr = malloc(args->array.len * sizeof(atom_p));
					
					for(size_t i = 1; i < atom->array.len; i++) {
						atom_p evaled_arg = eval(atom->array.ptr[i], env);
						if (evaled_arg->type == T_ERROR)
							return evaled_arg;
						args->array.ptr[i - 1] = evaled_arg;
					}
					
					return func->builtin(args, env);
				} else if (func->type == T_SYNTAX) {
					args->array.len = atom->array.len - 1;
					args->array.ptr = atom->array.ptr + 1;
					
					for(size_t i = 0; i < args->array.len; i++) {
						if (args->array.ptr[i]->type == T_ERROR)
							return args->array.ptr[i];
					}
					
					return func->syntax(args, env);
				} else if (func->type == T_LAMBDA) {
					if ( func->lambda.args->array.len != atom->array.len - 1 )
						return error_atom("eval: a lambda needs to be called with exactly as many args as defined");
					
					atom_p lambda_env = calloc(1, sizeof(atom_t));
					lambda_env->type = T_ENV;
					lambda_env->obj.parent = func->lambda.env;
					
					for(size_t i = 1; i < atom->array.len; i++) {
						atom_p arg_name = func->lambda.args->array.ptr[i - 1];
						if (arg_name->type != T_SYM)
							return error_atom("eval: in the definition of an lambda an argument has to be a symbol");
						atom_p evaled_arg = eval(atom->array.ptr[i], env);
						if (evaled_arg->type == T_ERROR)
							return evaled_arg;
						hash_set(lambda_env, arg_name->sym, evaled_arg);
					}
					
					return eval(func->lambda.body, lambda_env);
				}
			}
			return error_atom("eval: unknown atom in function slot");
		
		default:
			return error_atom("eval: got unknown atom");
	}
}


//
// Builtins and syntax atoms
//

atom_p syntax_set(atom_p args, atom_p env) {
	if ( !(args->array.len == 2 && args->array.ptr[0]->type == T_SYM) )
		return error_atom("set!: need exactly 2 args, first has to be a symbol");
	
	hash_set(env, args->array.ptr[0]->sym, eval(args->array.ptr[1], env));
	return nil_atom;
}

atom_p syntax_quote(atom_p args, atom_p env) {
	if (args->array.len == 1)
		return args->array.ptr[0];
	return error_atom("quote: only supports one arg");
}

atom_p syntax_if(atom_p args, atom_p env) {
	if (args->array.len != 3)
		return error_atom("if: needs exactly 3 args");
	
	atom_p cond = eval(args->array.ptr[0], env);
	if (cond->type == T_FALSE || cond->type == T_NIL)
		return eval(args->array.ptr[2], env);
	else
		return eval(args->array.ptr[1], env);
}

atom_p syntax_lambda(atom_p args, atom_p env) {
	if ( !(args->array.len == 2 && args->array.ptr[0]->type == T_ARRAY && args->array.ptr[1]->type == T_ARRAY) )
		return error_atom("lambda: needs exactly 2 args, both have to be arrays");
	
	atom_p atom = malloc(sizeof(atom_t));
	atom->type = T_LAMBDA;
	atom->lambda.args = args->array.ptr[0];
	atom->lambda.body = args->array.ptr[1];
	atom->lambda.env = env;
	
	return atom;
}


atom_p builtin_add(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return nil_atom;
	
	atom_p result = malloc(sizeof(atom_t));
	result->type = T_NUM;
	result->num = args->array.ptr[0]->num + args->array.ptr[1]->num;
	return result;
}

atom_p builtin_sub(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return nil_atom;
	
	atom_p result = malloc(sizeof(atom_t));
	result->type = T_NUM;
	result->num = args->array.ptr[0]->num - args->array.ptr[1]->num;
	return result;
}

atom_p builtin_mul(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return nil_atom;
	
	atom_p result = malloc(sizeof(atom_t));
	result->type = T_NUM;
	result->num = args->array.ptr[0]->num * args->array.ptr[1]->num;
	return result;
}

atom_p builtin_div(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return nil_atom;
	
	atom_p result = malloc(sizeof(atom_t));
	result->type = T_NUM;
	result->num = args->array.ptr[0]->num / args->array.ptr[1]->num;
	return result;
}

atom_p builtin_eq(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return error_atom("= needs 2 number args");
	
	atom_p result = malloc(sizeof(atom_t));
	result->type = (args->array.ptr[0]->num == args->array.ptr[1]->num) ? T_TRUE : T_FALSE;
	return result;
}

atom_p builtin_lt(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return error_atom("< needs 2 number args");
	
	atom_p result = malloc(sizeof(atom_t));
	result->type = (args->array.ptr[0]->num < args->array.ptr[1]->num) ? T_TRUE : T_FALSE;
	return result;
}

atom_p builtin_gt(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return error_atom("> needs 2 number args");
	
	atom_p result = malloc(sizeof(atom_t));
	result->type = (args->array.ptr[0]->num > args->array.ptr[1]->num) ? T_TRUE : T_FALSE;
	return result;
}

atom_p builtin_not(atom_p args, atom_p env) {
	if (args->array.len != 1)
		return error_atom("! needs one arg");
	
	atom_p result = malloc(sizeof(atom_t));
	result->type = (args->array.ptr[0]->type == T_FALSE || args->array.ptr[0]->type == T_NIL) ? T_TRUE : T_FALSE;
	return result;
}



int main() {
	// Init global stuff
	nil_atom = malloc(sizeof(atom_t));
	nil_atom->type = T_NIL;
	true_atom = malloc(sizeof(atom_t));
	true_atom->type = T_TRUE;
	false_atom = malloc(sizeof(atom_t));
	false_atom->type = T_FALSE;
	
	base_env = calloc(1, sizeof(atom_t));
	base_env->type = T_ENV;
	
	hash_set(base_env, "set!", syntax_atom(syntax_set));
	hash_set(base_env, "quote", syntax_atom(syntax_quote));
	hash_set(base_env, "if", syntax_atom(syntax_if));
	hash_set(base_env, "lambda", syntax_atom(syntax_lambda));
	
	hash_set(base_env, "+", builtin_atom(builtin_add));
	hash_set(base_env, "-", builtin_atom(builtin_sub));
	hash_set(base_env, "*", builtin_atom(builtin_mul));
	hash_set(base_env, "/", builtin_atom(builtin_div));
	
	hash_set(base_env, "=", builtin_atom(builtin_eq));
	hash_set(base_env, "<", builtin_atom(builtin_lt));
	hash_set(base_env, ">", builtin_atom(builtin_gt));
	hash_set(base_env, "!", builtin_atom(builtin_not));
	
	// Read eval print loop (REPL)
	atom_p atom = NULL;
	
	while (true) {
		printf("> ");
		atom = read(stdin);
		if (atom == NULL)
			break;  // EOF
		
		atom = eval(atom, base_env);
		
		print(stdout, atom);
		printf("\n");
	}
	
	return 0;
}