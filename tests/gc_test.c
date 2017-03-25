#define _GNU_SOURCE
#include <stdio.h>

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"

#include "../gc.c"


void test_gc_init_and_cleanup() {
	lvm_p lvm = lvm_gc_init();
	
	st_check_not_null(lvm);
	st_check_not_null(lvm->nil_atom);
	st_check_not_null(lvm->true_atom);
	st_check_not_null(lvm->false_atom);
	
	lvm_gc_cleanup(lvm);
}

void test_gc_alloc() {
	lvm_p lvm = lvm_gc_init();
	
	// Allo atom without data
	lvm_atom_p num_atom = lvm_gc_alloc_atom(lvm, LVM_T_NUM);
	st_check_not_null(num_atom);
	st_check_msg(num_atom == (void*)lvm->gc.new_space.last + sizeof(lvm_gc_region_t), "num atom wasn't allocated where it should be");
	st_check_int(num_atom->type, LVM_T_NUM);
	
	st_check_not_null(lvm->gc.new_space.last);
	st_check(lvm->gc.new_space.first == lvm->gc.new_space.last);
	
	// Alloc atom with data
	size_t data_size = 13;
	lvm_atom_p str_atom = lvm_gc_alloc_atom(lvm, LVM_T_STR);
	st_check_not_null(str_atom);
	st_check_msg(str_atom == (void*)num_atom + lvm_gc_atom_infos[LVM_T_NUM].size, "str atom wasn't allocated where it should be");
	str_atom->str = lvm_gc_alloc_data(lvm, data_size);
	st_check_msg(str_atom->str == (void*)lvm->gc.new_space.last + LVM_GC_REGION_SIZE - data_size, "str data wasn't allocated where it should be");
	
	// Fill up one space to force allocation in a new region
	while ( lvm->gc.new_space.last->free_bytes > 4*1024 ) {
		lvm_atom_p filler_atom = lvm_gc_alloc_atom(lvm, LVM_T_STR);
		st_check_not_null(filler_atom);
		filler_atom->str = lvm_gc_alloc_data(lvm, 4*1024);
		st_check_not_null(filler_atom->str);
	}
	
	// This allocation should create a new region
	lvm_atom_p new_region_atom = lvm_gc_alloc_atom(lvm, LVM_T_STR);
	st_check_not_null(new_region_atom);
	new_region_atom->str = lvm_gc_alloc_data(lvm, 4*1024);
	st_check_not_null(new_region_atom->str);
	
	st_check_int(lvm->gc.collect_on_next_possibility, true);
	st_check_not_null(lvm->gc.new_space.last);
	st_check(lvm->gc.new_space.first != lvm->gc.new_space.last);
	st_check(lvm->gc.new_space.first->next == lvm->gc.new_space.last);
	
	lvm_gc_cleanup(lvm);
}

void test_gc_collect() {
	lvm_p lvm = lvm_gc_init();
	
	// Allocate some atoms we want to survive
	lvm_atom_p atom_a = lvm_gc_alloc_atom(lvm, LVM_T_NUM);
	st_check_not_null(atom_a);
	atom_a->num = 13;
	
	const char* str_b = "hello atom b!";
	lvm_atom_p atom_b = lvm_gc_alloc_atom(lvm, LVM_T_STR);
	st_check_not_null(atom_b);
	atom_b->str = lvm_gc_alloc_data(lvm, strlen(str_b)+1);
	st_check_not_null(atom_b->str);
	strcpy(atom_b->str, str_b);
	
	lvm_atom_p atom_c = lvm_gc_alloc_atom(lvm, LVM_T_PAIR);
	st_check_not_null(atom_c);
	atom_c->first = atom_a;
	atom_c->rest = atom_b;
	
	// Allocate about 32 MiByte of garbage
	for(size_t i = 0; i < 1024 * 8; i++) {
		lvm_atom_p garbage_atom = lvm_gc_alloc_atom(lvm, LVM_T_STR);
		st_check_not_null(garbage_atom);
		garbage_atom->str = lvm_gc_alloc_data(lvm, 4*1024);
		st_check_not_null(garbage_atom->str);
	}
	
	lvm_gc_collect(lvm, (lvm_atom_p*[]){
		&atom_a,
		&atom_b,
		&atom_c,
		NULL
	}, (lvm_env_p[]){
		NULL
	});
	
	st_check_int(atom_a->type, LVM_T_NUM);
	st_check_int(atom_a->num, 13);
	
	st_check_int(atom_b->type, LVM_T_STR);
	st_check_str(atom_b->str, str_b);
	
	st_check_int(atom_c->type, LVM_T_PAIR);
	st_check(atom_c->first == atom_a);
	st_check(atom_c->rest == atom_b);
	
	lvm_gc_cleanup(lvm);
}


int main() {
	st_run(test_gc_init_and_cleanup);
	st_run(test_gc_alloc);
	st_run(test_gc_collect);
	return st_show_report();
}