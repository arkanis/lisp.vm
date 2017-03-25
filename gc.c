// For mmap() flag MAP_ANONYMOUS
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "internals.h"


//
// Garbage collector stuff
//


struct lvm_gc_region_s {
	lvm_gc_region_p next;
	uint16_t size_in_64k_chunks;
	uint16_t flags;
	uint32_t free_offset;
	uint32_t free_bytes;
};

// lvm_gc_region_t.flags
#define LVM_GC_DONT_MOVE    (0 << 1)





typedef void   (*lvm_gc_collect_child_t)(lvm_p lvm, lvm_atom_p* child_atom);
typedef void   (*lvm_gc_child_collector_t)(lvm_p lvm, lvm_atom_p atom, lvm_gc_collect_child_t collect_child);
typedef size_t (*lvm_gc_get_data_t)(lvm_p lvm, lvm_atom_p atom, void** data_ptr);
typedef struct {
	uint32_t size;
	lvm_gc_child_collector_t child_collector;
	lvm_gc_get_data_t        get_data;
} lvm_gc_atom_info_t, *lvm_gc_atom_info_p;


lvm_p lvm_gc_init();
void lvm_gc_cleanup(lvm_p lvm);
lvm_atom_p lvm_gc_alloc(lvm_p lvm, lvm_atom_type_t type, size_t data_size, void** data_ptr);
lvm_atom_p lvm_gc_alloc_atom(lvm_p lvm, lvm_atom_type_t type);
void*      lvm_gc_alloc_data(lvm_p lvm, size_t size);
void lvm_gc_invalidate_data_in_region(lvm_gc_region_p region);
lvm_gc_region_p lvm_gc_allocate_region(size_t size, uint16_t flags);

void lvm_gc_free_region(lvm_gc_region_p region);

void       lvm_gc_ensure_free_bytes_in_space(lvm_gc_space_p space, size_t size, bool* added_new_regiony);
lvm_atom_p lvm_gc_alloc_atom_from_space(lvm_gc_space_p space, lvm_atom_type_t type, bool* added_new_region);
void*      lvm_gc_alloc_data_from_space(lvm_gc_space_p space, size_t data_size, bool* added_new_region);
lvm_atom_p lvm_gc_alloc_from_space(lvm_gc_space_p space, lvm_atom_type_t type, size_t data_size, void** data_ptr, bool* added_new_region);

void lvm_gc_pair_child_collector(lvm_p lvm, lvm_atom_p atom, lvm_gc_collect_child_t collect_child);
size_t lvm_gc_get_str_data(lvm_p lvm, lvm_atom_p atom, void** data_ptr);

lvm_gc_atom_info_t lvm_gc_atom_infos[] = {
	[LVM_T_NIL]     = {.size = offsetof(struct lvm_atom_s, type)    + sizeof(lvm_atom_type_t)    },
	[LVM_T_TRUE]    = {.size = offsetof(struct lvm_atom_s, type)    + sizeof(lvm_atom_type_t)    },
	[LVM_T_FALSE]   = {.size = offsetof(struct lvm_atom_s, type)    + sizeof(lvm_atom_type_t)    },
	[LVM_T_NUM]     = {.size = offsetof(struct lvm_atom_s, num)     + sizeof(int64_t)            },
	[LVM_T_SYM]     = {.size = offsetof(struct lvm_atom_s, str)     + sizeof(char*),             .get_data = lvm_gc_get_str_data },
	[LVM_T_STR]     = {.size = offsetof(struct lvm_atom_s, str)     + sizeof(char*),             .get_data = lvm_gc_get_str_data },
	[LVM_T_PAIR]    = {.size = offsetof(struct lvm_atom_s, rest)    + sizeof(lvm_atom_p),        .child_collector = lvm_gc_pair_child_collector },
	[LVM_T_LAMBDA]  = {.size = offsetof(struct lvm_atom_s, env)     + sizeof(lvm_env_p)          },
	[LVM_T_BUILTIN] = {.size = offsetof(struct lvm_atom_s, builtin) + sizeof(lvm_builtin_func_t) },
	[LVM_T_SYNTAX]  = {.size = offsetof(struct lvm_atom_s, syntax)  + sizeof(lvm_syntax_func_t)  },
	[LVM_T_ERROR]   = {.size = offsetof(struct lvm_atom_s, str)     + sizeof(char*),             .get_data = lvm_gc_get_str_data }
};

#define LVM_GC_64K          (65536)
#define LVM_GC_REGION_SIZE  (16*1024*1024)



lvm_p lvm_gc_init() {
	lvm_gc_region_p first_uncollected_region = lvm_gc_allocate_region(LVM_GC_REGION_SIZE, LVM_GC_DONT_MOVE);
	lvm_gc_space_t uncollected = (lvm_gc_space_t){
		.first = first_uncollected_region,
		.last  = first_uncollected_region
	};
	
	lvm_p lvm = lvm_gc_alloc_data_from_space(&uncollected, sizeof(lvm_t), NULL);
	
	lvm->gc = (lvm_gc_t){
		.uncollected = uncollected,
		.new_space = { NULL, NULL },
		.old_space = { NULL, NULL },
		.collect_on_next_possibility = false
	};
	lvm->nil_atom   = lvm_gc_alloc_atom_from_space(&uncollected, LVM_T_NIL,   NULL);
	lvm->true_atom  = lvm_gc_alloc_atom_from_space(&uncollected, LVM_T_TRUE,  NULL);
	lvm->false_atom = lvm_gc_alloc_atom_from_space(&uncollected, LVM_T_FALSE, NULL);
	
	return lvm;
}

void lvm_gc_cleanup(lvm_p lvm) {
	lvm_gc_region_p next = NULL;
	
	for(lvm_gc_region_p r = lvm->gc.new_space.first; r != NULL; r = next) {
		next = r->next;
		lvm_gc_free_region(r);
	}
	
	for(lvm_gc_region_p r = lvm->gc.old_space.first; r != NULL; r = next) {
		next = r->next;
		lvm_gc_free_region(r);
	}
	
	// Free the uncollected region last since it provides the memory for the
	// lvm_p context struct.
	for(lvm_gc_region_p r = lvm->gc.uncollected.first; r != NULL; r = next) {
		next = r->next;
		lvm_gc_free_region(r);
	}
}

lvm_atom_p lvm_gc_alloc(lvm_p lvm, lvm_atom_type_t type, size_t data_size, void** data_ptr) {
	return lvm_gc_alloc_from_space(&lvm->gc.new_space, type, data_size, data_ptr, &lvm->gc.collect_on_next_possibility);
}

lvm_atom_p lvm_gc_alloc_atom(lvm_p lvm, lvm_atom_type_t type) {
	return lvm_gc_alloc_atom_from_space(&lvm->gc.new_space, type, &lvm->gc.collect_on_next_possibility);
}

void* lvm_gc_alloc_data(lvm_p lvm, size_t size) {
	return lvm_gc_alloc_data_from_space(&lvm->gc.new_space, size, &lvm->gc.collect_on_next_possibility);
}


void lvm_gc_collect_atom(lvm_p lvm, lvm_atom_p* atom);

void lvm_gc_collect(lvm_p lvm, lvm_atom_p* survivers[], lvm_env_p envs[]) {
	// Swap spaces
	lvm_gc_space_t temp = lvm->gc.new_space;
	lvm->gc.new_space = lvm->gc.old_space;
	lvm->gc.old_space = temp;
	
	// Root: Atoms on the argument stack
	for(size_t i = 0; i < lvm->arg_stack_length; i++) {
		lvm_gc_collect_atom(lvm, &lvm->arg_stack_ptr[i]);
	}
	
	// Root: Argument atoms of the collect function that have to survive
	for(size_t i = 0; survivers[i] != NULL; i++) {
		lvm_gc_collect_atom(lvm, survivers[i]);
	}
	
	// Root: Environments passed to the collector function
	for(size_t i = 0; envs[i] != NULL; i++) {
		// Go up the parent chain for this environment
		for(lvm_env_p env = envs[i]; env != NULL; env = env->parent) {
			for(lvm_dict_it_p it = lvm_dict_start(&env->bindings); it != NULL; it = lvm_dict_next(&env->bindings, it)) {
				//const char* key = it->key;
				lvm_gc_collect_atom(lvm, &it->value);
			}
			
		}
	}
	
	// Invalidate regions in old space
	for(lvm_gc_region_p r = lvm->gc.old_space.first; r != NULL; r = r->next) {
		lvm_gc_invalidate_data_in_region(r);
	}
}

void lvm_gc_collect_atom(lvm_p lvm, lvm_atom_p* atom) {
	lvm_atom_type_t type = (*atom)->type;
	
	// We already collected the atom in question. Just patch the pointer to point directly to the new atom (instead of a
	// forward pointer).
	if (type == LVM_T_FORWARD_PTR) {
		*atom = (*atom)->new_atom;
		return;
	}
	
	// Copy atom to new space
	size_t data_size = 0;
	void* old_data_ptr = NULL;
	if (lvm_gc_atom_infos[type].get_data != NULL)
		data_size = lvm_gc_atom_infos[type].get_data(lvm, *atom, &old_data_ptr);
	
	void* new_data_ptr = NULL;
	lvm_atom_p new_atom = lvm_gc_alloc_from_space(&lvm->gc.new_space, type, data_size, &new_data_ptr, NULL);
	size_t atom_size = lvm_gc_atom_infos[type].size;
	memcpy(new_atom, *atom, atom_size);
	
	// Copy data to new space
	if (data_size > 0) {
		memcpy(new_data_ptr, old_data_ptr, data_size);
		// TODO: Find a proper way to patch the data pointer of the atom
		new_atom->str = new_data_ptr;
	}
	
	// Write forward pointer
	(*atom)->type = LVM_T_FORWARD_PTR;
	(*atom)->new_atom = new_atom;
	
	// Patch old atom pointer directly to new atom
	*atom = new_atom;
	
	// Iterate over child atoms of atom in new space
	if (lvm_gc_atom_infos[type].child_collector != NULL)
		lvm_gc_atom_infos[type].child_collector(lvm, new_atom, lvm_gc_collect_atom);
}



lvm_gc_region_p lvm_gc_allocate_region(size_t size, uint16_t flags) {
	size_t size_in_64k_chunks = (size + (LVM_GC_64K - 1)) / LVM_GC_64K;
	lvm_gc_region_p region = mmap(NULL, size_in_64k_chunks * LVM_GC_64K, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (region == MAP_FAILED) {
		// TODO: error handling
		exit(1);
	}
	
	*region = (lvm_gc_region_t){
		.next = NULL,
		.size_in_64k_chunks = size_in_64k_chunks,
		.flags = flags,
		.free_offset = sizeof(lvm_gc_region_t),
		.free_bytes = (size_in_64k_chunks * LVM_GC_64K) - sizeof(lvm_gc_region_t)
	};
	
	return region;
}

void lvm_gc_invalidate_data_in_region(lvm_gc_region_p region) {
	// Don't invalidate the header struct at the start of the region. So leave one 4k page there.
	void* ptr = (void*)(((size_t)region + sizeof(lvm_gc_region_t) + 4096 - 1) / 4096 * 4096);
	madvise(ptr, region->size_in_64k_chunks * LVM_GC_64K - 4096, MADV_DONTNEED);
}

void lvm_gc_free_region(lvm_gc_region_p region) {
	munmap(region, region->size_in_64k_chunks * LVM_GC_64K);
}


void lvm_gc_ensure_free_bytes_in_space(lvm_gc_space_p space, size_t size, bool* added_new_region) {
	lvm_gc_region_p region = space->last;
	if ( region == NULL || region->free_bytes < size ) {
		// Not enough space, we need to add another region to this space
		lvm_gc_region_p new_region = lvm_gc_allocate_region(LVM_GC_REGION_SIZE, 0);
		
		// Wire new region into space
		if (region != NULL)
			region->next = new_region;
		else
			space->first = new_region;
		space->last = new_region;
		
		// Set marker if the caller wants to know
		if (added_new_region)
			*added_new_region = true;
	}
}

lvm_atom_p lvm_gc_alloc_atom_from_space(lvm_gc_space_p space, lvm_atom_type_t type, bool* added_new_region) {
	uint32_t atom_size = lvm_gc_atom_infos[type].size;
	lvm_gc_ensure_free_bytes_in_space(space, atom_size, added_new_region);
	lvm_gc_region_p region = space->last;
	
	lvm_atom_p atom = (void*)region + region->free_offset;
	atom->type = type;
	region->free_offset += atom_size;
	region->free_bytes  -= atom_size;
	
	return atom;
}

void* lvm_gc_alloc_data_from_space(lvm_gc_space_p space, size_t data_size, bool* added_new_region) {
	lvm_gc_ensure_free_bytes_in_space(space, data_size, added_new_region);
	lvm_gc_region_p region = space->last;
	
	void* data_ptr = (void*)region + region->free_offset + region->free_bytes - data_size;
	region->free_bytes -= data_size;
	
	return data_ptr;
}

lvm_atom_p lvm_gc_alloc_from_space(lvm_gc_space_p space, lvm_atom_type_t type, size_t data_size, void** data_ptr, bool* added_new_region) {
	uint32_t atom_size = lvm_gc_atom_infos[type].size;
	lvm_gc_ensure_free_bytes_in_space(space, atom_size + data_size, added_new_region);
	
	lvm_gc_region_p region = space->last;
	lvm_atom_p atom = (void*)region + region->free_offset;
	atom->type = type;
	region->free_offset += atom_size;
	region->free_bytes  -= atom_size;
	
	if (data_size > 0) {
		*data_ptr = (void*)region + region->free_offset + region->free_bytes - data_size;
		region->free_bytes -= data_size;
	}
	
	return atom;
}






void lvm_gc_pair_child_collector(lvm_p lvm, lvm_atom_p atom, lvm_gc_collect_child_t collect_child) {
	collect_child(lvm, &atom->first);
	collect_child(lvm, &atom->rest);
}

size_t lvm_gc_get_str_data(lvm_p lvm, lvm_atom_p atom, void** data_ptr) {
	*data_ptr = atom->str;
	return strlen(atom->str) + 1;
}