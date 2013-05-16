#pragma once

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

/**

Data layout:

| hash  | uint64_t
| key   |  int64_t const char*
| value | content type
| ...   |
|-------|
| hash  |

*/

typedef struct {
	size_t length, capacity;
	size_t value_size;
	uint8_t type;
	void* slots;
} hash_t, *hash_p;
typedef void* hash_elem_t;

#define HASH_NUMERIC_KEYS  0
#define HASH_STRING_KEYS   1


#define hash_of(type)              hash_new(5, sizeof(type))
#define hash_with(capacity, type)  hash_new(capacity, sizeof(type))

hash_p hash_new(size_t capacity, size_t value_size);
void   hash_resize(hash_p hash, size_t new_capacity);
void   hash_destroy(hash_p hash);


#define hash_put(hash, key, type, value)  ( *((type*)hash_put_ptr(hash, key)) = (value) )
#define hash_get(hash, key, type)         ( *((type*)hash_get_ptr(hash, key)) )

void* hash_put_ptr(hash_p hash, int64_t key);
void* hash_get_ptr(hash_p hash, int64_t key);
void  hash_remove(hash_p hash, int64_t key);
bool  hash_contains(hash_p hash, int64_t key);


hash_elem_t hash_start(hash_p hash);
hash_elem_t hash_next(hash_p hash, hash_elem_t element);

#define hash_key(element)             ( *hash_slot_key_ptr(element) )
#define hash_value(element, type)     ( *((type*)hash_slot_value_ptr(element)) )

void hash_remove_elem(hash_p hash, hash_elem_t element);


#define hash_slot_size(hash)        ( sizeof(uint64_t) + sizeof(int64_t) + hash->value_size )
#define hash_slot_ptr(hash, index)  ( (char*)hash->slots + hash_slot_size(hash) * index )
#define hash_slot_hash_ptr(slot)    ( (uint64_t*)slot )
#define hash_slot_key_ptr(slot)     ( (int64_t*)((char*)slot + sizeof(uint64_t)) )
#define hash_slot_value_ptr(slot)   ( (void*)((char*)slot + sizeof(uint64_t) + sizeof(int64_t)) )



#define dict_of(type)              dict_new(5, sizeof(type))
#define dict_with(capacity, type)  dict_new(capacity, sizeof(type))

hash_p dict_new(size_t capacity, size_t value_size);
void   dict_resize(hash_p hash, size_t new_capacity);
void   dict_destroy(hash_p hash);

#define dict_put(hash, key, type, value)  ( *((type*)dict_put_ptr(hash, key)) = (value) )
#define dict_get(hash, key, type)         ( *((type*)dict_get_ptr(hash, key)) )

void* dict_put_ptr(hash_p hash, const char* key);
void* dict_get_ptr(hash_p hash, const char* key);
void  dict_remove(hash_p hash, const char* key);
bool  dict_contains(hash_p hash, const char* key);


hash_elem_t dict_start(hash_p hash);
hash_elem_t dict_next(hash_p hash, hash_elem_t element);

#define dict_key(element)             ( *dict_slot_key_ptr(element) )
#define dict_value(element, type)     ( *((type*)dict_slot_value_ptr(element)) )

void hash_remove_elem(hash_p hash, hash_elem_t element);


#define dict_slot_size(hash)        ( sizeof(uint64_t) + sizeof(const char *) + hash->value_size )
#define dict_slot_ptr(hash, index)  ( (char*)hash->slots + hash_slot_size(hash) * index )
#define dict_slot_hash_ptr(slot)    ( (uint64_t*)slot )
#define dict_slot_key_ptr(slot)     ( (const char **)((char*)slot + sizeof(uint64_t)) )
#define dict_slot_value_ptr(slot)   ( (void*)((char*)slot + sizeof(uint64_t) + sizeof(const char *)) )


/*
h = hash_new(10, float);
hash_put(h, 42, float, 3.141);
hash_get(h, 42, float);
for(void* ptr = hash_start(h); ptr != NULL; ptr = hash_next(h, ptr)){
	int64_t k = hash_key(h, ptr);
	hash_value(h, ptr, float);
}
for(void* ptr = hash_end(h); ptr != NULL; ptr = hash_prev(h, ptr)){
	int64_t k = hash_key(h, ptr);
	hash_value(h, ptr, float);
}
hash_includes(h, 64);
hash_destroy(h);


d = dict_new(10, float);
dict_put(d, "foo", float, 3.141);
dict_get(d, "foo", float);
for(void* ptr = dict_start(d); ptr != NULL; ptr = dict_next(d, ptr)){
	const char* k = dict_key(d, ptr);
	dict_value(d, ptr, float);
}
dict_includes(d, "test");
dict_destroy(d);

// Utility methods

hash_contains(hash, key)
hash_merge(hash_a, hash_b);
*/