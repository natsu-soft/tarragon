#include "free_list.h"
#include <stdlib.h>
#include "scratch.h"
#include "../error_macros.h"
#include "../pointer_memes.h"
#include "../benchmark.h"
/* #include "sorted_list.h" */

typedef struct free_node_t
{
	struct free_node_t* next;
	size_t value;
} free_node_t;

typedef struct alloc_header_t
{
	size_t size;
	size_t padding;
} alloc_header_t;

void list_insert(free_node_t** list, free_node_t* node)
{
	node->next = *list;
	*list = node;
}

void list_remove(free_node_t** list, free_node_t* node)
{
	for (; *list != NULL; *list = (*list)->next)
	{
		if (*list == node)
		{
			*list = node->next;
		}
	}
}

free_node_t* list_extract_first_gte(free_node_t** list, const size_t size)
{
	for (; *list != NULL; *list = (*list)->next)
	{
		free_node_t* res = *list;
		if (res->value >= size)
		{
			*list = res->next;
			return res;
		}
	}
	return NULL;
}

// O(n)
free_node_t* list_extract_best_gte(free_node_t** list, const size_t size)
{
	free_node_t* res = NULL;
	free_node_t** res_ptr = &res;
	for (; *list != NULL; *list = (*list)->next)
	{
		free_node_t* temp = *list;
		if (temp->value >= size && (res == NULL || temp->value < res->value))
		{
			res_ptr = list;
			*list = res->next;
		}
	}
	if (res)
	{
		*res_ptr = res->next;
	}
	return res;
}

void list_point_relative(free_node_t* list)
{
	free_node_t* temp;
	for (; list != NULL; list = temp)
	{
		temp = list->next;
		if (temp != NULL)
		{
			point_relative(&list->next);
		}
	}
}

void list_point_absolute(free_node_t* list)
{
	for (; list != NULL; list = list->next)
	{
		if (list->next != NULL)
		{
			point_absolute(&list->next);
		}
	}
}

void print_chunks(const free_list_t* fl)
{
	for (free_node_t* l = fl->head; l != NULL; l = l->next)
	{
		printf("%zu%s", l->value, l->next != NULL ? ", " : "\n");
	}
}

static const size_t align(const void* ptr, const size_t pow2)
{
	const size_t modulo = (size_t)ptr & (pow2 - 1);
	return modulo > 0 ? pow2 - modulo : 0;
}

void free_list_init(free_list_t* fl, size_t bytes)
{
	fl->cap = bytes;
	fl->allocation = malloc(bytes);
	free_list_clear(fl);
}

void free_list_grow(free_list_t* fl, size_t bytes)
{
	const size_t size0 = fl->cap;
	assertf(bytes >= size0, "It's called GROW.");
	free_list_point_relative(fl);
	uint8_t* memory = realloc(fl->allocation, bytes);
	assertf(memory != NULL, "failed to allocate %zu bytes or %zuGB", bytes, bytes / (1024 * 1024 * 1024));
	fl->allocation = memory;
	free_list_point_absolute(fl);
	const uint8_t* m1 = memory + bytes;
	const size_t delta = bytes - size0;
	fl->cap = bytes;
	if (delta >= 2 * sizeof(free_node_t))
	{
		const uint8_t* ptr = memory + size0;
		const size_t shift = align(ptr, sizeof(free_node_t));
		free_node_t* list = ptr + shift;
		assert(memory <= list && list < m1);
		list->value = delta - shift;
		list->next = NULL;
		list_insert(&fl->head, list);
	}
}

void* free_list_alloc(free_list_t* fl, size_t bytes)
{
	const size_t header_size = sizeof(alloc_header_t);
	free_node_t* node = list_extract_first_gte (&fl->head, bytes + header_size); // O(n)
	if (node != NULL)
	{
		const size_t total = node->value;
		alloc_header_t* header = node;
		uint8_t* ptr = header + 1;
		header->size = bytes;
		header->padding = total - bytes - sizeof *header;
		if (header->padding >= 2 * sizeof *node)
		{
			const size_t shift = align(ptr + bytes, sizeof(free_node_t));
			node = ptr + bytes + shift;
			node->value = header->padding - shift;
			list_insert(&fl->head, node);
			header->padding = shift;
		}
		return ptr;
	}
	return NULL;
}

void* free_list_calloc(free_list_t* fl, size_t bytes)
{
	void* res = free_list_alloc(fl, bytes);
	if (res != NULL)
	{
		memset(res, 0x00, bytes);
	}
	return res;
}


void* free_list_dealloc(free_list_t* fl, void* ptr, size_t bytes)
{
	if (ptr != NULL)
	{
		alloc_header_t* header = (alloc_header_t*)ptr - 1;
		assertf(bytes <= header->size, "DEALLOC not REALLOC");
		header->padding += header->size - bytes;
		header->size = bytes;
		if (header->padding >= 2 * sizeof(free_node_t))
		{
			const size_t shift = align(ptr + bytes, sizeof(free_node_t));
			free_node_t* node = (uint8_t*)ptr + bytes + shift;
			node->value = header->padding - shift;
			list_insert(&fl->head, node);
			header->padding = shift;
		}
	}
	return ptr;
}

void free_list_free(free_list_t* fl, void* ptr)
{
	const alloc_header_t* header = (alloc_header_t*)ptr - 1;
	/* const size_t freed_size = header->size + header->padding + sizeof *header; */
	free_node_t* node = header;
	node->value = header->size + header->padding + sizeof *header;
	list_insert(&fl->head, node);
}

void free_list_clear(free_list_t* fl)
{
	free_node_t* list = fl->allocation;
	list->next = NULL;
	list->value = fl->cap;
	fl->head = list;
}

static int compare_address(const void* a, const void* b)
{
	return (size_t)(*(const free_node_t**)a) - (size_t)(*(const free_node_t**)b); // overflow? I barely know her
}

void free_list_coalesce(free_list_t* fl)
{
	if (fl->head != NULL)
	{
		size_t num = 0;
		for(free_node_t* l = fl->head; l != NULL; l = l->next) { ++num; }
		free_node_t** list = scratch_alloc(num * sizeof *list);
		// 1. populate the list
		size_t i = 0;
		for(free_node_t* l = fl->head; l != NULL; l = l->next)
		{
			list[i++] = l;
		}
		// 2. sort by address
		qsort(list, num, sizeof *list, compare_address);
		// 3. if sizes touch (or are less than sizeof(size_t) away) then & delete the latter
		size_t swap = 1;
		const size_t merge_size = sizeof(size_t);
		for (i = swap; i < num; ++i)
		{
			const size_t j = swap - 1;
			const size_t end_addr = (size_t)list[j] + list[j]->value;
			const size_t diff = (size_t)list[i] - end_addr;
			if (diff > merge_size)
			{
				swap++;
			}
			else
			{
				list[j]->value += list[i]->value + diff;
			}
		}
		num = swap;
		(*list)->next = NULL;
		fl->head = NULL;
		// reinsert
		for (i = 0; i <= num; ++i)
		{
			list_insert(&fl->head, list[i]);
		}
		/* scratch_free(list); */
	}
}

void free_list_point_relative(free_list_t* fl)
{
	if (fl->head != NULL)
	{
		list_point_relative(fl->head);
		point_relative_offset(&fl->head, fl->allocation);
		fl->valid_head = 0x01;
	}
	else
	{
		fl->valid_head = 0x00;
	}
}

void free_list_point_absolute(free_list_t* fl)
{
	if (fl->valid_head)
	{
		point_absolute_offset(&fl->head, fl->allocation);
		list_point_absolute(fl->head);
	}
}
void free_list_serialize(void* destination, free_list_t* fl)
{
	uint8_t* dest = destination;
	free_list_point_relative(fl);
	const size_t len = offsetof(free_list_t, allocation);
	memcpy(dest, fl, len);
	memcpy(dest + len, fl->allocation, fl->cap);
	free_list_point_absolute(fl);
}

void free_list_deserialize(free_list_t* fl, const void* source)
{
	uint8_t* src = source;
	if (fl->allocation != NULL)
	{
		free(fl->allocation);
	}
	const size_t len = offsetof(free_list_t, allocation);
	memcpy(fl, src, len);
	fl->allocation = malloc(fl->cap);
	assert(fl->allocation != NULL);
	memcpy(fl->allocation, src + len, fl->cap);
	free_list_point_absolute(fl);
}


size_t free_list_coverage(free_list_t* fl)
{
	size_t coverage = 0;
	for (free_node_t* list = fl->head; list != NULL; list = list->next)
	{
		coverage += list->value;
	}
	return coverage;
}

/* const size_t free_list_largest_allocation(const free_list_t* fl) */
/* { */
/* 	return fl->head ? fl->head->size : 0; */
/* } */

int free_list_test(int argc, char** argv)
{
	#define INIT_SIZE 1024
	free_list_t* fl = alloca(sizeof *fl);
	free_list_init(fl, INIT_SIZE);
	void* buffer = malloc(INIT_SIZE);
	printf("  -- allocation: ");
	char* foo = free_list_alloc(fl, 512);
	assert(foo != NULL);
	foo = free_list_alloc(fl, 256);
	assert(foo != NULL);
	foo = free_list_alloc(fl, 128);
	assert(foo != NULL);
	foo = free_list_alloc(fl, 64);
	assert(foo != NULL);
	assert(fl->head == NULL);
	printf("GOOD!\n");
	/* print_chunks(fl); */

	printf("  -- clear: ");
	free_list_clear(fl);
	printf("GOOD!\n");
	/* print_chunks(fl); */

	printf("  -- free: ");
	foo = free_list_alloc(fl, 512);
	char* bar = free_list_alloc(fl, 256);
	free_list_free(fl, foo);
	free_list_free(fl, bar);
	printf("GOOD!\n");
	/* print_chunks(fl); */

	printf("  -- translation: ");
	for (int i = 0; i < 99; ++i)
	{
		void* ptr = free_list_alloc(fl, 1);
		if (ptr == NULL)
		{
			break;
		}
		free_list_free(fl, ptr);
	}
	free_list_point_relative(fl);
	memcpy(buffer, fl->allocation, fl->cap);
	free(fl->allocation);
	fl->allocation = buffer;
	free_list_point_absolute(fl);
	printf("GOOD!\n");
	/* print_chunks(fl); */

	fflush(stdout);
	free_list_clear(fl);
	/* print_chunks(fl); */
	alloc_header_t* header = free_list_alloc(fl, 200);
	printf("  -- dealloc: ");
	header = free_list_dealloc(fl, header, 32);
	header = header - 1;
	assert(header->size == 32);
	free_list_free(fl, header + 1);
	printf("GOOD!\n");
	/* print_chunks(fl); */
	/* foo = NULL; */
	/* buffer = NULL; */

	printf("  -- grow: ");
	const char spinner[4] = { '\\', '-', '/', '|' };
	fflush(stdout);
/* #define LIMIT 0x400000000 */
#define LIMIT 0x800000000 // slightly less because this machine only has 16gigs ram LMAO
	const float resize_scale = 1.618f;
	const size_t resize_overhead = 4096;
	for (int i = 0; i <= 999999; ++i)
	{
		printf("[%c]", spinner[i % 4]);
		printf("\b\b\b");
		const size_t new_size = resize_scale * fl->cap + resize_overhead;
		if (new_size > LIMIT)
		{
			break;
		}
		free_list_grow(fl, new_size);
	}
	printf("GOOD!\n");
	/* print_chunks(fl); */

#define STRESS_TEST 10000000
#ifdef STRESS_TEST
	printf("  -- stress test:\n");
	printf("   + allocating: ");
	fflush(stdout);
	buffer = malloc(fl->cap);
	assert(buffer); // do this first in case you run out of memory
	printf("GOOD\n");
	printf("   + fragmenting: ");
	fflush(stdout);
	LONGTIME(for (unsigned int i = 0; i < STRESS_TEST; ++i)
	{
		printf("%06.2f%%", 100.0f * (float)i / (float)STRESS_TEST);
		fflush(stdout);
		void* ptr = free_list_alloc(fl, 64);
		if (ptr != NULL)
		{
			ptr = free_list_dealloc(fl, ptr, 16);
			free_list_free(fl, ptr);
		}
			printf("\b\b\b\b\b\b\b");
	}
	printf("GOOD!    \n");)
	printf("   + caching: ");
	fflush(stdout);
	size_t num = 0;
	for (free_node_t* l = fl->head; l != NULL; l = l->next)
	{
		num++;
	}
	size_t* values = malloc(num * sizeof *values);
	int i = 0;
	for (free_node_t* l = fl->head; l != NULL; l = l->next)
	{
		fflush(stdout);
		values[i++] = l->value;
	}
	printf("GOOD\n");
	printf("   + translating: ");
	fflush(stdout);
	free_list_point_relative(fl);
	memcpy(buffer, fl->allocation, fl->cap);
	free(fl->allocation);
	fl->allocation = buffer;
	free_list_point_absolute(fl);
	printf("GOOD\n");
	printf("   + checking: ");
	fflush(stdout);
	i = 0;
	for (free_node_t* l = fl->head; l != NULL; l = l->next)
	{
		assert(values[i++] == l->value);
	}
	free(values);
	printf("GOOD\n");
	printf("  > succesfully translated %zu accessible memory chunks\n", num);
	printf("  > size: %zu GB (%zu bytes)\n", fl->cap / 0x40000000, fl->cap);
#endif
	/* print_chunks(fl); */
	printf("  -- coalescing: ");
	free_list_coalesce(fl);
	printf("GOOD!      \n");
	/* print_chunks(fl); */
	free(fl->allocation);

	return 0;
}
