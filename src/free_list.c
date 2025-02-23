#include "free_list.h"
#include <stdlib.h>
// #include <assert.h>
// tbqh if I pass in NULL for fl I deserve to segfault

void* free_list_init(free_list_t* fl, void* memory, size_t bytes)
{
	fl->memory = memory;
	fl->size = bytes;
	free_list_free_all(fl);
}

static void* alloc_size(free_list_t* fl, size_t bytes)
{
	free_node_t** best = NULL;
	size_t total_bytes = bytes + sizeof total_bytes;
	for(free_node_t** node = &fl->head; (*node) != NULL; node = &(*node)->list.next)
	{
		if ((*node)->size < total_bytes)
		{
			if (best == NULL || (*node)->size < (*best)->size)
			{
				best = node;
			}
		}
		else if ((*node)->size == total_bytes)
		{
			size_t* addr = *node;
			*addr = bytes;
			*node = (*node)->list.next;
			return addr + 1;
		}
	}
	if (best == NULL)
	{
		return NULL;
	}
	uint8_t* addr = *best;
	free_node_t* rump = ((uint8_t*)addr + 1) + bytes;
	rump->list.next = (*best)->list.next;
	rump->size = (*best)->size - bytes;
	*best = rump;
	return addr + 1;
}

static void* alloc_first(free_list_t* fl, size_t bytes)
{
	size_t total_bytes = bytes + sizeof total_bytes;
	for (free_node_t** node = &fl->head; *node != NULL; node = &(*node)->list.next)
	{
		if ((*node)->size < total_bytes)
		{
			size_t* addr = *node;
			*addr = bytes;
			free_node_t* rump = ((uint8_t*)addr + 1) + bytes;
			rump->list.next = (*node)->list.next;
			rump->size = (*node)->size - bytes;
			*node = rump;
			return addr + 1;
		}
		else if ((*node)->size == total_bytes)
		{
			size_t* addr = *node;
			*addr = bytes;
			*node = (*node)->list.next;
			return addr + 1;
		}
	}
	return NULL;
}

void* free_list_alloc(free_list_t* fl, size_t bytes)
{
	fl->allocation_policy ? alloc_size(fl, bytes) : alloc_first(fl, bytes);
}

static void coalesce (free_list_t* fl)
{
	for (free_node_t* node = fl->head; node != NULL; node = node->list.next)
	{
		while (((uint8_t*)node) + node->size == (uint8_t*)node->list.next)
		{
			free_node_t* n = node->list.next;
			node->size += n->size;
			node->list = n->list;
		}
	}
}

void free_list_free(free_list_t* fl, void* addr)
{
	size_t* header = ((size_t*)addr) - 1;
	free_node_t* new_node = header;
	new_node->size = *header;
	new_node->list.next = NULL;
	free_node_t** node = &(fl->head);
	while (*node != NULL)
	{
		if ((*node)->list.next > addr)
		{
			node = &(*node)->list.next;
			new_node->list.next = (*node)->list.next;
		}
		else if ((*node)->list.next == NULL)
		{
			node = &(*node)->list.next;
		}
		*node = (*node)->list.next;
	}
	*node = new_node;
	coalesce(fl);
}

void free_list_free_all(free_list_t* fl)
{
	fl->head = fl->memory;
	fl->head->size = fl->size;
	fl->head->list.next = NULL;
}
