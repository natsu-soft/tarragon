#ifndef FREE_LIST_H
#define FREE_LIST_H

#include <stddef.h>
#include <stdint.h>


typedef struct free_list_t
{
	// NOTE: order matters for serialization because I am lazy >:^)
	struct free_node_t* head;
	size_t size;
	uint8_t* memory;
	uint8_t valid_head;
} free_list_t;

void free_list_init(free_list_t* fl, size_t bytes);

void free_list_grow(free_list_t* fl, size_t bytes);

void* free_list_alloc(free_list_t* fl, size_t bytes);

void* free_list_dealloc(free_list_t* fl, void* ptr, size_t bytes);

void free_list_free(free_list_t* fl, void* ptr);

void free_list_clear(free_list_t* fl);

void free_list_coalesce(free_list_t* fl);

void free_list_point_relative(free_list_t* fl);

void free_list_point_absolute(free_list_t* fl);

void free_list_serialize(void* destination, free_list_t* fl);

void free_list_deserialize(free_list_t* fl, const void* source);

size_t free_list_coverage(free_list_t* fl);

const size_t free_list_largest_allocation(const free_list_t* fl);

#endif /* End FREE_LIST_H */
