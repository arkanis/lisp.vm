/**

Extends the previous segmented Baker GC by implementing the actual garbage collection. For that purpose it introduces a
stack where eval can push the atoms and environments that are currently evaled. We need that because this is a recursive
interpreter and the GC needs to know tht atoms and environments on the call stack. Otherwise it would collect these
atoms.

Code works for collects in the top level eval but crashed as soon as a collect is triggered in a recursive eval() call.
This is because the GC doesn't know and can't patch all the atom references on the C stack.

- The code is ugly as hell.
- Not covered by any test cases.

**/

// For mmap() MAP_ANONYMOUS
#define _GNU_SOURCE

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
	T_LAMBDA,
	T_FORWARD_PTR
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
		atom_p forward_ptr;
	};
};

atom_p nil_atom = NULL;
atom_p true_atom = NULL;
atom_p false_atom = NULL;
atom_p base_env = NULL;


//
// Memory management
//

atom_p* stack;
size_t sp, stack_length;

void stack_init() {
	stack_length = 8*1024;
	stack = mmap(NULL, stack_length * sizeof(stack[0]), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	sp = 0;
}

void stack_push(atom_p atom) {
	if (sp >= stack_length) {
		// Stack full, grow it
		size_t new_stack_length = stack_length * 2;
		stack = mremap(stack, stack_length * sizeof(stack[0]), new_stack_length * sizeof(stack[0]), MREMAP_MAYMOVE);
		stack_length = new_stack_length;
	}
	
	stack[sp++] = atom;
}



typedef struct gc_segment_s gc_segment_t, *gc_segment_p;
struct gc_segment_s {
	size_t size;
	size_t filled;
	gc_segment_p next;
};

gc_segment_p uncollected_space, new_space, old_space;
gc_segment_p current_uncollected_segment, current_new_segment;
size_t alloced_segments_since_last_collect;
bool in_collect = false;

#define GC_SEGMENT_SIZE 1*1024*1024

gc_segment_p gc_alloc_segment() {
	gc_segment_p segment = mmap(NULL, GC_SEGMENT_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	segment->size = GC_SEGMENT_SIZE;
	segment->filled = sizeof(gc_segment_t);
	segment->next = NULL;
	return segment;
}

void* gc_alloc_uncollected(size_t size);
void* gc_alloc(size_t size);

void gc_init() {
	alloced_segments_since_last_collect = 0;
	
	uncollected_space = gc_alloc_segment();
	current_uncollected_segment = uncollected_space;
	new_space = gc_alloc_segment();
	current_new_segment = new_space;
	
	old_space = NULL;
	
	nil_atom = gc_alloc_uncollected(sizeof(atom_t));
	nil_atom->type = T_NIL;
	true_atom = gc_alloc_uncollected(sizeof(atom_t));
	true_atom->type = T_TRUE;
	false_atom = gc_alloc_uncollected(sizeof(atom_t));
	false_atom->type = T_FALSE;
	
	base_env = gc_alloc(sizeof(atom_t));
	memset(base_env, 0, sizeof(atom_t));
	base_env->type = T_ENV;
}

void* gc_alloc_uncollected(size_t size) {
	if (current_uncollected_segment->filled + size > current_uncollected_segment->size) {
		// No space, need new segment
		gc_segment_p next = gc_alloc_segment();
		current_uncollected_segment->next = next;
		current_uncollected_segment = next;
	}
	
	void* ptr = (void*)current_uncollected_segment + current_uncollected_segment->filled;
	current_uncollected_segment->filled += size;
	return ptr;
}

void* gc_alloc(size_t size) {
	if (current_new_segment->filled + size > current_new_segment->size) {
		// No space, we need to allocate from another segment
		if (current_new_segment->next) {
			// There is another segment after this one in this space, so use it
			current_new_segment = current_new_segment->next;
			current_new_segment->filled = sizeof(gc_segment_t);
		} else if (old_space && !in_collect) {
			// Steal a segment from old space
			gc_segment_p segment = old_space;
			old_space = old_space->next;
			segment->next = NULL;
			segment->filled = sizeof(gc_segment_t);
			
			current_new_segment->next = segment;
			current_new_segment = segment;
		} else {
			// Nothing in old space, alloc new segment
			gc_segment_p segment = gc_alloc_segment();
			current_new_segment->next = segment;
			current_new_segment = segment;
		}
		
		alloced_segments_since_last_collect++;
	}
	
	void* ptr = (void*)current_new_segment + current_new_segment->filled;
	current_new_segment->filled += size;
	current_new_segment->filled += current_new_segment->filled % 4;
	
	//fprintf(stderr, "gc_alloc, atom %p size %zu filled %zu segment size %zu\n", ptr, size, current_new_segment->filled, current_new_segment->size);
	
	return ptr;
}

void* gc_alloc_zeroed(size_t size) {
	void* ptr = gc_alloc(size);
	memset(ptr, 0, size);
	return ptr;
}

void* gc_realloc(void* ptr, size_t old_size, size_t new_size) {
	if (ptr + old_size == (void*)current_new_segment + current_new_segment->filled && current_new_segment->filled - old_size + new_size <= current_new_segment->size) {
		// Special case: ptr is the last memory block in the segment and we have to space to extend it, so do it
		current_new_segment->filled = current_new_segment->filled - old_size + new_size;
		return ptr;
	} else {
		// In all other cases do a normal alloc
		void* new_ptr = gc_alloc(new_size);
		memcpy(new_ptr, ptr, old_size);
		return new_ptr;
	}
}

void gc_collect_atom(atom_p* atom, uint32_t level);
void gc_print_info();

void gc_collect_if_necessary(atom_p* current_eval_atom, atom_p* current_eval_env) {
	if (alloced_segments_since_last_collect == 0)
		return;
	
	fprintf(stderr, "gc: starting collect\n");
	gc_print_info();
	
	
	in_collect = true;
	
	// Allocate one segment for the old space if nothing is there. Otherwise the gc_alloc() will fail after swapping the
	// spaces.
	if (old_space == NULL)
		old_space = gc_alloc_segment();
	
	// Swap spaces so we alloc new space during collection from the other space
	gc_segment_p temp = new_space;
	new_space = old_space;
	old_space = temp;
	current_new_segment = new_space;
	
	// roots: base env, all arguments and environments in the eval call chain (all stuff on the stack)
	for(size_t i = 0; i < sp; i++)
		gc_collect_atom(&stack[i], 0);
	
	gc_collect_atom(&base_env, 0);
	gc_collect_atom(current_eval_atom, 0);
	gc_collect_atom(current_eval_env, 0);
	
	// Invalidate pages in old space
	//long pagesize = 4096;
	for(gc_segment_p segment = old_space; segment != NULL; segment = segment->next) {
		segment->filled = sizeof(gc_segment_t);
		//madvise(segment + pagesize, segment->size - pagesize, MADV_DONTNEED);
	}
	
	alloced_segments_since_last_collect = 0;
	in_collect = false;
	
	fprintf(stderr, "gc: finished collect\n");
	gc_print_info();
}

obj_slot_p hash_start(atom_p obj);
obj_slot_p hash_next(atom_p obj, obj_slot_p it);
void hash_set(atom_p obj, const char* key, atom_p value);
void print(FILE* output, atom_p atom);

void gc_collect_atom(atom_p* atom, uint32_t level) {
	fprintf(stderr, "%*satom %p: ", (int)level*2, "", *atom);
	print(stderr, *atom);
	fprintf(stderr, "\n");
	
	atom_p old_atom = *atom;
	atom_type_t type = old_atom->type;
	
	// Don't move atoms from the uncollected space to the new space. They just stay where they are.
	if (old_atom == nil_atom || old_atom == true_atom || old_atom == false_atom)
		return;
	
	// We already collected the atom in question. Just patch the pointer to point directly to the new atom (instead of a
	// forward pointer).
	if (type == T_FORWARD_PTR) {
		*atom = old_atom->forward_ptr;
		return;
	}
	
	// Copy atom to new space
	atom_p new_atom = gc_alloc(sizeof(atom_t));
	memcpy(new_atom, old_atom, sizeof(atom_t));
	
	// Copy atom data to new space
	switch (type) {
		case T_STR: {
			size_t data_size = strlen(old_atom->str) + 1;
			new_atom->str = gc_alloc(data_size);
			memcpy(new_atom->str, old_atom->str, data_size);
			break;
		}
		case T_ARRAY: {
			size_t data_size = old_atom->array.len * sizeof(old_atom->array.ptr[0]);
			new_atom->array.ptr = gc_alloc(data_size);
			memcpy(new_atom->array.ptr, old_atom->array.ptr, data_size);
			break;
		}
		case T_OBJ: case T_ENV: {
			size_t data_size = old_atom->obj.cap * sizeof(old_atom->obj.slots[0]);
			new_atom->obj.slots = gc_alloc(data_size);
			memcpy(new_atom->obj.slots, old_atom->obj.slots, data_size);
			break;
		}
		case T_ERROR: {
			size_t data_size = strlen(old_atom->error) + 1;
			new_atom->error = gc_alloc(data_size);
			memcpy(new_atom->error, old_atom->error, data_size);
			break;
		}
		
		case T_SYM:
			// Do nothing, symbol string allocated in uncollected space
		case T_NIL: case T_TRUE: case T_FALSE: case T_NUM:
		case T_BUILTIN: case T_SYNTAX: case T_LAMBDA:
			// Do nothing
		case T_FORWARD_PTR:
			// Impossible, handled at function start
			break;
	}
	
	// Write forward pointer
	old_atom->type = T_FORWARD_PTR;
	old_atom->forward_ptr = new_atom;
	
	// Patch pointer to old atom to point directly to new atom
	*atom = new_atom;
	
	// Iterate over child atoms of atom in new space
	switch (type) {
		case T_ARRAY: {
			for(size_t i = 0; i < new_atom->array.len; i++)
				gc_collect_atom(&new_atom->array.ptr[i], level + 1);
			break;
		}
		case T_OBJ: case T_ENV: {
			// Move all value atoms in the slots to the other space (skip empty slots)
			for(obj_slot_p it = hash_start(new_atom); it != NULL; it = hash_next(new_atom, it))
				gc_collect_atom(&it->value, level + 1);
			
			// Move all parent objects/environments
			if (new_atom->obj.parent != NULL)
				gc_collect_atom(&new_atom->obj.parent, level + 1);
			break;
		}
		case T_LAMBDA: {
			gc_collect_atom(&new_atom->lambda.args, level + 1);
			gc_collect_atom(&new_atom->lambda.body, level + 1);
			gc_collect_atom(&new_atom->lambda.env, level + 1);
			break;
		}
		
		case T_SYM:
			// Do nothing, symbol string allocated in uncollected space
		case T_STR: case T_ERROR:
		case T_NIL: case T_TRUE: case T_FALSE: case T_NUM:
		case T_BUILTIN: case T_SYNTAX:
			// Do nothing
		case T_FORWARD_PTR:
			// Impossible, handled at function start
			break;
	}
}

void gc_print_info() {
	size_t segment_count = 0, used_mem = 0;
	for(gc_segment_p s = new_space; s != NULL; s = s->next) {
		segment_count++;
		used_mem += s->filled;
	}
	fprintf(stderr, "gc: used_mem %zu bytes in %zu segments\n", used_mem, segment_count);
	
	fprintf(stderr, "  new space:\n");
	for(gc_segment_p s = new_space; s != NULL; s = s->next)
		fprintf(stderr, "    segment %p (%zu of %zu bytes used), next %p\n", s, s->filled, s->size, s->next);
	fprintf(stderr, "  old space:\n");
	for(gc_segment_p s = old_space; s != NULL; s = s->next)
		fprintf(stderr, "    segment %p (%zu of %zu bytes used), next %p\n", s, s->filled, s->size, s->next);
}




atom_p error_atom(char* message) {
	atom_p atom = gc_alloc(sizeof(atom_t));
	atom->type = T_ERROR;
	atom->error = message;
	return atom;
}

atom_p syntax_atom(eval_proc_t func) {
	atom_p atom = gc_alloc(sizeof(atom_t));
	atom->type = T_SYNTAX;
	atom->builtin = func;
	return atom;
}

atom_p builtin_atom(eval_proc_t func) {
	atom_p atom = gc_alloc(sizeof(atom_t));
	atom->type = T_BUILTIN;
	atom->builtin = func;
	return atom;
}


//
// Hash table for objects and environments
//

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
	new_obj.obj.slots = gc_alloc_zeroed(new_capacity * sizeof(new_obj.obj.slots[0]));
	
	for(obj_slot_p it = hash_start(obj); it != NULL; it = hash_next(obj, it)) {
		hash_set(&new_obj, it->key, it->value);
	}
	
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
	
	char* str = NULL;
	if ( fscanf(input, "\"%m[^\"]\"", &str) == 1 ) {
		atom_p atom = gc_alloc(sizeof(atom_t));
		atom->type = T_STR;
		atom->str = gc_alloc(strlen(str) + 1);
		strcpy(atom->str, str);
		free(str);
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
		
		atom_p atom = gc_alloc(sizeof(atom_t));
		atom->type = T_NUM;
		atom->num = value;
		return atom;
	} else if (c == '(') {
		atom_p atom = gc_alloc(sizeof(atom_t));
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
			
			atom->array.ptr = gc_realloc(atom->array.ptr, atom->array.len * sizeof(atom->array.ptr[0]), (atom->array.len + 1) * sizeof(atom->array.ptr[0]));
			atom->array.len += 1;
			atom->array.ptr[atom->array.len - 1] = item;
		}
	} else if (c == '{') {
		atom_p atom = gc_alloc_zeroed(sizeof(atom_t));
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
			atom = gc_alloc_uncollected(sizeof(atom_t));
			atom->type = T_SYM;
			atom->sym = gc_alloc_uncollected(strlen(str) + 1);
			strcpy(atom->sym, str);
			free(str);
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
		case T_FORWARD_PTR:
			fprintf(output, "<forward_ptr %p>", atom->forward_ptr);
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
				gc_collect_if_necessary(&atom, &env);
				
				if (atom->array.len == 0)
					return error_atom("eval: array needs at least one elment to be evaled");
				
				atom_p func = eval(atom->array.ptr[0], env);
				if (func->type == T_ERROR)
					return func;
				if ( !(func->type == T_BUILTIN || func->type == T_SYNTAX || func->type == T_LAMBDA) )
					return error_atom("eval: only builtin, syntax and lambda atoms are allowed in the function slot");
				
				atom_p args = gc_alloc(sizeof(atom_t));
				args->type = T_ARRAY;
				
				if (func->type == T_BUILTIN) {
					args->array.len = atom->array.len - 1;
					args->array.ptr = gc_alloc(args->array.len * sizeof(atom_p));
					
					for(size_t i = 1; i < atom->array.len; i++) {
						atom_p evaled_arg = eval(atom->array.ptr[i], env);
						if (evaled_arg->type == T_ERROR)
							return evaled_arg;
						args->array.ptr[i - 1] = evaled_arg;
					}
					
					size_t old_sp = sp;
					stack_push(env);
					stack_push(args);
					atom_p result = func->builtin(args, env);
					sp = old_sp;
					return result;
				} else if (func->type == T_SYNTAX) {
					args->array.len = atom->array.len - 1;
					args->array.ptr = atom->array.ptr + 1;
					
					for(size_t i = 0; i < args->array.len; i++) {
						if (args->array.ptr[i]->type == T_ERROR)
							return args->array.ptr[i];
					}
					
					size_t old_sp = sp;
					stack_push(env);
					stack_push(args);
					atom_p result = func->syntax(args, env);
					sp = old_sp;
					return result;
				} else if (func->type == T_LAMBDA) {
					if ( func->lambda.args->array.len != atom->array.len - 1 )
						return error_atom("eval: a lambda needs to be called with exactly as many args as defined");
					
					atom_p lambda_env = gc_alloc_zeroed(sizeof(atom_t));
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
					
					size_t old_sp = sp;
					stack_push(env);
					stack_push(args);
					atom_p result = eval(func->lambda.body, lambda_env);
					sp = old_sp;
					return result;
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
	
	atom_p atom = gc_alloc(sizeof(atom_t));
	atom->type = T_LAMBDA;
	atom->lambda.args = args->array.ptr[0];
	atom->lambda.body = args->array.ptr[1];
	atom->lambda.env = env;
	
	return atom;
}


atom_p builtin_add(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return nil_atom;
	
	atom_p result = gc_alloc(sizeof(atom_t));
	result->type = T_NUM;
	result->num = args->array.ptr[0]->num + args->array.ptr[1]->num;
	return result;
}

atom_p builtin_sub(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return nil_atom;
	
	atom_p result = gc_alloc(sizeof(atom_t));
	result->type = T_NUM;
	result->num = args->array.ptr[0]->num - args->array.ptr[1]->num;
	return result;
}

atom_p builtin_mul(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return nil_atom;
	
	atom_p result = gc_alloc(sizeof(atom_t));
	result->type = T_NUM;
	result->num = args->array.ptr[0]->num * args->array.ptr[1]->num;
	return result;
}

atom_p builtin_div(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return nil_atom;
	
	atom_p result = gc_alloc(sizeof(atom_t));
	result->type = T_NUM;
	result->num = args->array.ptr[0]->num / args->array.ptr[1]->num;
	return result;
}

atom_p builtin_eq(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return error_atom("= needs 2 number args");
	
	return (args->array.ptr[0]->num == args->array.ptr[1]->num) ? true_atom : false_atom;
}

atom_p builtin_lt(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return error_atom("< needs 2 number args");
	
	return (args->array.ptr[0]->num < args->array.ptr[1]->num) ? true_atom : false_atom;
}

atom_p builtin_gt(atom_p args, atom_p env) {
	if (args->array.len != 2 || args->array.ptr[0]->type != T_NUM || args->array.ptr[1]->type != T_NUM)
		return error_atom("> needs 2 number args");
	
	return (args->array.ptr[0]->num > args->array.ptr[1]->num) ? true_atom : false_atom;
}

atom_p builtin_not(atom_p args, atom_p env) {
	if (args->array.len != 1)
		return error_atom("! needs one arg");
	
	return (args->array.ptr[0]->type == T_FALSE || args->array.ptr[0]->type == T_NIL) ? true_atom : false_atom;
}


atom_p builtin_gc_info(atom_p args, atom_p env) {
	gc_print_info();
	return nil_atom;
}

atom_p builtin_gc_collect(atom_p args, atom_p env) {
	alloced_segments_since_last_collect = 10;
	gc_collect_if_necessary(&nil_atom, &env);
	return nil_atom;
}





int main() {
	// Init stuff
	stack_init();
	gc_init();
	
	// Fill base environment
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
	
	hash_set(base_env, "gc_info", builtin_atom(builtin_gc_info));
	hash_set(base_env, "gc_collect", builtin_atom(builtin_gc_collect));
	
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