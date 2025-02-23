#ifndef FREE_LIST_H
#define FREE_LIST_H

#include "list.h"
#include <stddef.h>
#include <stdint.h>

typedef enum ALLOCATION_POLICY
{
	FIRST_FIT = 0x00,
	BEST_FIT,
} ALLOCATION_POLICY;

typedef struct free_node_t
{
	list_t list;
	size_t size;
} free_node_t;

typedef struct free_list_t
{
	free_node_t* head;
	uint8_t* memory;
	size_t size;
	uint8_t allocation_policy;
} free_list_t;

void* free_list_init(free_list_t* fl, void* memory, size_t bytes);

void* free_list_alloc(free_list_t* fl, size_t bytes);

void free_list_free(free_list_t* fl, void* addr);

void free_list_free_all(free_list_t* fl);


#endif /* End FREE_LIST_H */
