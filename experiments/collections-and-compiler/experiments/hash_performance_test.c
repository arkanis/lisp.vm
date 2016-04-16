#include <stdlib.h>
#include <stdio.h>
#include "../collections/hash.h"
#include "../timer.h"

int main(){
	size_t insert_count = 1000000;
	size_t lookup_count = 1000000;
	
	hash_p hash = hash_of(float);
	printf("starting to insert elements...\n");
	timeval_t mark = time_now();
	
	srand(42);
	for(size_t i = 0; i < insert_count; i++)
		hash_put(hash, rand(), float, 1.0);
	
	double insert_duration = time_since(mark);
	printf("%lfms for inserting %zu elements. %lfms per element.\n", insert_duration, insert_count, insert_duration / insert_count);
	
	size_t hit_count = 0;
	srand(42);
	for(size_t i = 0; i < lookup_count; i++){
		if ( hash_get_ptr(hash, rand() + i % 2) != NULL )
			hit_count++;
	}
	
	double lookup_duration = time_since(mark);
	printf("%lfms for lookup of %zu elements. %lfms per element. %zu hits.\n", lookup_duration, lookup_count, lookup_duration / lookup_count, hit_count);
	
	hash_destroy(hash);
	
	return 0;
}