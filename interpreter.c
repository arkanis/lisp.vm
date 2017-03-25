#include "internals.h"

lvm_p lvm_new() {
#	ifdef GC_REGION_BAKER
	lvm_p lvm = lvm_gc_init();
#	else
	lvm_p lvm = malloc(sizeof(lvm_t));
#	endif
	
	if (lvm == NULL)
		return NULL;
	
	lvm_mem_init(lvm);
	lvm->base_env = lvm_new_base_env(lvm);
	
	return lvm;
}

void lvm_destroy(lvm_p lvm) {
	lvm_env_destroy(lvm, lvm->base_env);
	lvm->base_env = NULL;
	lvm_mem_free(lvm);
	free(lvm);
}

lvm_env_p lvm_base_env(lvm_p lvm) {
	return lvm->base_env;
}