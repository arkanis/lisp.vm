#include <stdlib.h>
#include "array.h"

array_p array_new(size_t length, size_t element_size){
	array_p array = malloc(sizeof(array_t));
	
	if (array == NULL)
		return NULL;
	
	array->length = length;
	array->element_size = element_size;
	array->data = malloc(length * element_size);
	
	if (array->data == NULL){
		free(array);
		return NULL;
	}
	
	return array;
}

void array_destroy(array_p array){
	free(array->data);
	free(array);
}

/**
 * Resize should return the pointer to the newly appended memory block on success.
 * NULL is returned when the array shrinks or the realloc failed.
 */
void* array_resize(array_p array, size_t new_length){
	void* reallocated_data = realloc(array->data, new_length * array->element_size);
	if (reallocated_data == NULL)
		return NULL;
	
	size_t old_length = array->length;
	array->length = new_length;
	array->data = reallocated_data;
	
	return (old_length < new_length) ? (char*)array->data + (old_length * array->element_size) : NULL;
}