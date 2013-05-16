#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "hash.h"

#define HASH_SLOT_FREE     0
#define HASH_SLOT_DELETED  UINT64_MAX

static ssize_t hash_search_hash(hash_p hashmap, int64_t key, uint64_t hash);
static inline hash_elem_t slot_at_or_after(hash_p hash, hash_elem_t slot);
static uint64_t hash_int64(int64_t key);
size_t snap_to_prime(size_t x);


hash_p hash_new(size_t capacity, size_t value_size){
	hash_p hash = malloc(sizeof(hash_t));
	
	if (hash == NULL)
		return NULL;
	
	hash->capacity = capacity;
	hash->value_size = value_size;
	hash->type = HASH_NUMERIC_KEYS;
	hash->length = 0;
	hash->slots = calloc(hash->capacity, hash_slot_size(hash));
	
	if (hash->slots == NULL){
		free(hash);
		return NULL;
	}
	
	return hash;
}

/*
hash_p dict_new(size_t capacity, size_t value_size){
	hash_p hash = malloc(sizeof(hash_t));
	
	if (hash == NULL)
		return NULL;
	
	hash->capacity = capacity;
	hash->value_size = value_size;
	hash->type = HASH_STRING_KEYS;
	hash->length = 0;
	hash->slots = calloc(hash->capacity, hash_slot_size(hash));
	
	if (hash->slots == NULL){
		free(hash);
		return NULL;
	}
	
	return hash;
}
*/

void hash_destroy(hash_p hash){
	free(hash->slots);
	free(hash);
}


hash_elem_t hash_start(hash_p hash){
	return slot_at_or_after(hash, hash->slots);
}

hash_elem_t hash_next(hash_p hash, hash_elem_t element){
	return slot_at_or_after(hash, (char*)element + hash_slot_size(hash));
}


void hash_resize(hash_p hash, size_t new_capacity){
	// Just in case: avoid to make the hashmap smaller than it can be
	if (new_capacity < hash->length)
		return;
	
	hash_t new_hash;
	new_hash.value_size = hash->value_size;
	new_hash.length = 0;
	new_hash.capacity = new_capacity;
	new_hash.slots = calloc(new_hash.capacity, hash_slot_size(hash));
	
	if (new_hash.slots == NULL)
		return;
	
	for(hash_elem_t elem = hash_start(hash); elem != NULL; elem = hash_next(hash, elem)){
		void* old_value_ptr = hash_slot_value_ptr(elem);
		void* new_value_ptr = hash_put_ptr(&new_hash, hash_key(elem));
		memcpy(new_value_ptr, old_value_ptr, hash->value_size);
	}
	
	free(hash->slots);
	*hash = new_hash;
}

void* hash_put_ptr(hash_p hashmap, int64_t key){
	if (hashmap->length + 1 > hashmap->capacity * 0.75)
		hash_resize(hashmap, snap_to_prime(hashmap->capacity * 2));
	
	uint64_t hash = hash_int64(key);
	ssize_t index = hash_search_hash(hashmap, key, hash);
	void* slot = NULL;
	
	// Key wasn't found. The return value is -(next_free_index + 1).
	if (index < 0) {
		index = -index - 1;
		slot = hash_slot_ptr(hashmap, index);
		
		*hash_slot_hash_ptr(slot) = hash;
		*hash_slot_key_ptr(slot) = key;
		hashmap->length++;
	} else {
		slot = hash_slot_ptr(hashmap, index);
	}
	
	return hash_slot_value_ptr(slot);
}

void* hash_get_ptr(hash_p hashmap, int64_t key){
	uint64_t hash = hash_int64(key);
	ssize_t index = hash_search_hash(hashmap, key, hash);
	if (index < 0)
		return NULL;
	return hash_slot_value_ptr( hash_slot_ptr(hashmap, index) );
}

void hash_remove(hash_p hashmap, int64_t key){
	uint64_t hash = hash_int64(key);
	ssize_t index = hash_search_hash(hashmap, key, hash);
	
	if (index < 0)
		return;
	
	void* slot = hash_slot_ptr(hashmap, index);
	*hash_slot_hash_ptr(slot) = HASH_SLOT_DELETED;
	hashmap->length--;
	
	if (hashmap->length < hashmap->capacity * 0.2)
		hash_resize(hashmap, snap_to_prime(hashmap->capacity / 2));
}

void hash_remove_elem(hash_p hash, hash_elem_t element){
	*hash_slot_hash_ptr(element) = HASH_SLOT_DELETED;
	hash->length--;
}

bool hash_contains(hash_p hash, int64_t key){
	return (hash_get_ptr(hash, key) != NULL);
}


/**
 * Search the hashmap for the specified key.
 * 
 * Return value >= 0: The key has been found and it's index is returned.
 * Return value <= -1: The key was not found. The index of a free slot -1 is returned
 *   as a negative number. This is either the first free slot in the probing sequence
 *   or the first slot marked as deleted.
 */
static ssize_t hash_search_hash(hash_p hashmap, int64_t key, uint64_t hash){
	uint64_t hash_at_index;
	size_t index = hash % hashmap->capacity;
	size_t probe_offset = 1;
	ssize_t first_deleted_index = -1;
	
	while(true) {
		void* slot = hash_slot_ptr(hashmap, index);
		hash_at_index = *hash_slot_hash_ptr(slot);
		
		if (first_deleted_index == -1 && hash_at_index == HASH_SLOT_DELETED) {
			first_deleted_index = index;
		} else if (hash_at_index == HASH_SLOT_FREE) {
			if (first_deleted_index != -1)
				return -(first_deleted_index + 1);
			else
				return -(index + 1);
		} else if ( *hash_slot_key_ptr(slot) == key ) {
			// TODO: For strings first compare the hash, then strcmp.
			return index;
		}
		
		index = (index + probe_offset * probe_offset) % hashmap->capacity;
		probe_offset++;
	}
}

// Partially optimized prime number finding algo taken from libc++ implementation posted
// by Howard Hinnant at http://stackoverflow.com/a/5694432

static inline bool is_prime(size_t x){
	for(size_t i = 3; true; i += 2){
		size_t q = x / i;
		if (q < i)
			return true;
		if (x % i == 0)
			return false;
	}
	
	return true;
}

size_t snap_to_prime(size_t x){
	// First search the precomputed list. The list contains the next primes after 2^x (starting with x = 2).
	size_t primes[] = {5, 11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853, 25717, 51437, 102877, 205759, 411527, 823117, 1646237, 3292489, 6584983};
	for(size_t i = 0; i < sizeof(primes) / sizeof(size_t); i++){
		if (primes[i] >= x)
			return primes[i];
	}
	
	// No match, compute next larger prime
	// Make x odd by setting the least significant bit (there are no even primes)
	x |= 1;
	
	// Loop odd numbers until we have a prime
	while( !is_prime(x) )
		x += 2;
	
	return x;
}

/**
 * Internal function that starts at the memory address `slot` and scans for the next
 * occupied slot (not free or deleted).
 */
static inline hash_elem_t slot_at_or_after(hash_p hash, hash_elem_t slot){
	for(char* ptr = slot; ptr < (char*)hash->slots + hash_slot_size(hash) * hash->capacity; ptr += hash_slot_size(hash)) {
		uint64_t slot_hash = *hash_slot_hash_ptr(ptr);
		if ( slot_hash != HASH_SLOT_FREE && slot_hash != HASH_SLOT_DELETED )
			return ptr;
	}
	
	return NULL;
}


/**
 * Hashing using the MurmurHash3
 * https://code.google.com/p/smhasher/wiki/MurmurHash3
 * 
 * Added holes in the hash function for the value 0 and UINT64_MAX. The
 * Hash function does not generate these values. Therefore they can be
 * used to signal free or deleted slots. The price for this is that the
 * values of 1 and UINT64_MAX - 1 have twice the probability to appear.
 */
static uint64_t hash_int64(int64_t key){
	uint64_t h = key;
	
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccd;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53;
	h ^= h >> 33;
	
	if (h == 0)
		h = 1;
	else if (h == UINT64_MAX)
		h = UINT64_MAX - 1;
	
	return h;
}