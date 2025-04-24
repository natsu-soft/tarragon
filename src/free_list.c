#include "free_list.h"
#include <stdlib.h>
#include "scratch.h"
#include "error_macros.h"
#include "pointer_memes.h"
#include "sorted_list.h"

typedef union alloc_header_t
{
	struct
	{
		size_t size;
		size_t padding;
	};
	uint8_t requisite_size[sizeof(sorted_list_t)];
} alloc_header_t;

void print_chunks(const free_list_t* fl)
{
	for (sorted_list_t* l = fl->head; l != NULL; l = l->next)
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
	fl->size = bytes;
	fl->memory = malloc(bytes);
	free_list_clear(fl);
}

void free_list_grow(free_list_t* fl, size_t bytes)
{
	const size_t size0 = fl->size;
	assertf(bytes >= size0, "It's called GROW.");
	free_list_point_relative(fl);
	uint8_t* memory = realloc(fl->memory, bytes);
	assertf(memory != NULL, "failed to allocate %zu bytes or %zuGB", bytes, bytes / (1024 * 1024 * 1024));
	fl->memory = memory;
	free_list_point_absolute(fl);
	const uint8_t* m1 = memory + bytes;
	const size_t delta = bytes - size0;
	fl->size = bytes;
	if (delta > sizeof(sorted_list_t) + sizeof(size_t))
	{
		const uint8_t* ptr = memory + size0;
		const size_t shift = align(ptr, sizeof(size_t));
		sorted_list_t* list = ptr + shift;
		assert(memory <= list && list < m1);
		list->value = delta - shift;
		list->next = NULL;
		sorted_list_insert(&fl->head, list);
	}
}

void* free_list_alloc(free_list_t* fl, size_t bytes)
{
	const size_t header_size = sizeof(alloc_header_t);
	if (fl->head != NULL && fl->head->value >= bytes + header_size)
	{
		sorted_list_t* head = sorted_list_extract(&fl->head);
		const size_t total = head->value;
		alloc_header_t* header = head;
		uint8_t* ptr = header + 1;
		header->size = bytes;
		header->padding = total - bytes - sizeof *header;
		if (header->padding > sizeof *head + sizeof(size_t))
		{
			const size_t shift = align(ptr + bytes, sizeof(size_t));
			sorted_list_t* cleave = ptr + bytes + shift;
			cleave->value = header->padding;
			cleave->value -= shift;
			cleave->next = NULL;
			sorted_list_insert(&fl->head, cleave);
			header->padding = shift;
		}
		return ptr;
	}
	return NULL;
}

void* free_list_dealloc(free_list_t* fl, void* ptr, size_t bytes)
{
	if (ptr != NULL)
	{
		alloc_header_t* header = (alloc_header_t*)ptr - 1;
		assertf(bytes <= header->size, "DEALLOC not REALLOC");
		header->padding += header->size - bytes;
		header->size = bytes;
		if (header->padding > sizeof(sorted_list_t) + sizeof(size_t))
		{
			const size_t shift = align(ptr + bytes, sizeof(size_t));
			sorted_list_t* list = (uint8_t*)ptr + bytes + shift;
			list->value = header->padding;
			list->value -= shift;
			list->next = NULL;
			sorted_list_insert(&fl->head, list);
			header->padding = shift;
		}
	}
	return ptr;
}

void* free_list_free(free_list_t* fl, void* ptr)
{
	const alloc_header_t* header = (alloc_header_t*)ptr - 1;
	/* const size_t freed_size = header->size + header->padding + sizeof *header; */
	sorted_list_t* list = header;
	list->value = header->size + header->padding + sizeof *header;
	list->next = NULL;
	sorted_list_insert(&fl->head, list);
	return list;
}

void free_list_clear(free_list_t* fl)
{
	sorted_list_t* list = fl->memory;
	list->next = NULL;
	list->value = fl->size;
	fl->head = list;
}

static int compare_address(const void* a, const void* b)
{
	return (size_t)(*(const sorted_list_t**)a) - (size_t)(*(const sorted_list_t**)b); // overflow? I barely know her
}

static int compare_values(const void* a, const void* b)
{
	return (*(const sorted_list_t**)a)->value - (*(const sorted_list_t**)b)->value; // overflow? I barely know her
}

void free_list_coalesce(free_list_t* fl)
{
	if (fl->head != NULL)
	{
		size_t num = 0;
		for(sorted_list_t* l = fl->head; l != NULL; l = l->next) { ++num; }
		const size_t list_size = num * sizeof(sorted_list_t**) + sizeof(sorted_list_t);
		sorted_list_t** list;
		const uint8_t based_flag = fl->head->value >= list_size;
		// D E V I L I S H T A C T I C S: ideally store the list within itself
		// NOTE: have to offset because you don't want to overwrite the struct fields whilestve ordering it, because you need that data
		if (based_flag)
		{
			printf("\n\nATTN: BASEDBASEDBASEDBASEDBASEDBASEDBASEDBASEDBASEDBASEDBASEDBASEDBASED\n");
			list = fl->head + 1;
		}
		else
		{
			// otherwise malloc :(
			printf("\n\nWARNING: CINGE\n");
			list = scratch_alloc(num * sizeof(sorted_list_t**)); // LMAO YOU CAN RUN OUT OF MEMORY HERE
			assert(list);
		}
		// 1. populate the list
		size_t i = 0;
		for(sorted_list_t* l = fl->head; l != NULL; l = l->next)
		{
			list[i++] = l;
		}
		// 2. sort by address
		qsort(list, num, sizeof(sorted_list_t**), compare_address);
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
		// 4. sort by increasing size
		qsort(list, num, sizeof(sorted_list_t**), compare_values);
		(*list)->next = NULL;
		fl->head = NULL;
		// reinsert
		for (i = 0; i < num; ++i)
		{
			sorted_list_insert(&fl->head, list[i]);
		}
		if (!based_flag)
		{
			scratch_free(list);
		}
	}
}

void free_list_point_relative(free_list_t* fl)
{
	if (fl->head != NULL)
	{
		sorted_list_point_relative(fl->head);
		point_relative_offset(&fl->head, fl->memory);
		fl->valid_translation = 0x01;
	}
	else
	{
		fl->valid_translation = 0x00;
	}
}

void free_list_point_absolute(free_list_t* fl)
{
	if (fl->valid_translation)
	{
		point_absolute_offset(&fl->head, fl->memory);
		sorted_list_point_absolute(fl->head);
	}
}
void free_list_serialize(void* destination, free_list_t* fl)
{
	uint8_t* dest = destination;
	free_list_point_relative(fl);
	const size_t len = offsetof(free_list_t, memory);
	memcpy(dest, fl, len);
	memcpy(dest + len, fl->memory, fl->size);
	free_list_point_absolute(fl);
}

void free_list_deserialize(free_list_t* fl, const void* source)
{
	uint8_t* src = source;
	if (fl->memory != NULL)
	{
		free(fl->memory);
	}
	const size_t len = offsetof(free_list_t, memory);
	memcpy(fl, src, len);
	fl->memory = malloc(fl->size);
	assert(fl->memory != NULL);
	memcpy(fl->memory, src + len, fl->size);
	free_list_point_absolute(fl);
}


size_t free_list_coverage(free_list_t* fl)
{
	size_t coverage = 0;
	for (sorted_list_t* list = fl->head; list != NULL; list = list->next)
	{
		coverage = list->value;
	}
	return coverage;
}

const size_t free_list_largest_allocation(const free_list_t* fl)
{
	return fl->head ? fl->head->value : 0;
}

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
	memcpy(buffer, fl->memory, fl->size);
	free(fl->memory);
	fl->memory = buffer;
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
#define LIMIT 0x100000000 // slightly less because this machine only has 16gigs ram LMAO
	const float resize_scale = 1.618f;
	const size_t resize_overhead = 4096;
	for (int i = 0; i <= 999999; ++i)
	{
		printf("[%c]", spinner[i % 4]);
		printf("\b\b\b");
		const size_t new_size = resize_scale * fl->size + resize_overhead;
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
	printf("  -- stress test: ");
	printf("allocating");
	buffer = malloc(fl->size);
	assert(buffer); // do this first in case you run out of memory
	printf("\b\b\b\b\b\b\b\b\b\b");
	printf("fragmenting: ");
	fflush(stdout);
	for (int i = 0; i < STRESS_TEST; ++i)
	{
		printf("%06.2f%%", 100.0f * (float)i / (float)STRESS_TEST);
		fflush(stdout);
		void* ptr = free_list_alloc(fl, 69);
		if (ptr == NULL)
		{
			printf("\b\b\b\b\b\b\b");
			break;
		}
		ptr = free_list_dealloc(fl, ptr, 17);
		ptr = free_list_free(fl, ptr);
		printf("\b\b\b\b\b\b\b");
	}
	printf("\b\b\b\b\b\b\b\b\b\b\b");
	printf("caching    ");
	fflush(stdout);
	size_t num = 0;
	for (sorted_list_t* l = fl->head; l != NULL; l = l->next)
	{
		num++;
	}
	size_t* values = malloc(num * sizeof *values);
	int i = 0;
	for (sorted_list_t* l = fl->head; l != NULL; l = l->next)
	{
		fflush(stdout);
		values[i++] = l->value;
	}
	printf("\b\b\b\b\b\b\b\b\b\b\b");
	printf("translating");
	fflush(stdout);
	free_list_point_relative(fl);
	memcpy(buffer, fl->memory, fl->size);
	free(fl->memory);
	fl->memory = buffer;
	free_list_point_absolute(fl);
	printf("\b\b\b\b\b\b\b\b\b\b\b");
	printf("checking   ");
	fflush(stdout);
	i = 0;
	for (sorted_list_t* l = fl->head; l != NULL; l = l->next)
	{
		assert(values[i++] == l->value);
	}
	free(values);
	printf("\b\b\b\b\b\b\b\b\b\b\b");
	printf("GOOD!      \n");
	printf("  > succesfully translated %zu accessible memory chunks\n", num);
	printf("  > size: %zu GB (%zu bytes)\n", fl->size / 0x40000000, fl->size);
#endif
	/* print_chunks(fl); */
	printf("  -- coalescing: ");
	free_list_coalesce(fl);
	printf("GOOD!      \n");
	/* print_chunks(fl); */
	free(fl->memory);

	return 0;
}
