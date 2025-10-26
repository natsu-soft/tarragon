#include "builtin.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "allocators/arena.h"
#include "hashmaps/ptr_map.h"
#include "allocators/scratch.h"
#include "allocators/workpad.h"

static int32_t hashmap_size(const int32_t num_entries)
{
	for(int8_t i = 30; i >= 0; --i)
	{
		const int32_t mask = 1u << i;
		if (num_entries & mask)
		{
			return ((num_entries & mask) & ~(mask - 1)) << 2;
		}
	}
	assert(0 && "failed to find a good size for hashmap");
	return -1;
}

/* static int32_t align64(const int32_t size) */
/* { */
/* 	const int32_t mod = size % 64; */
/* 	return size + (mod ? 64 - mod : 0); */
/* } */
static int32_t offset_push(arena_t* arena, const void* src, const int32_t bytes)
{
	const int32_t size0 = arena->size;
	arena_push(arena, src, bytes);
	return size0;
}


struct
{
	ptr_map_t tagged_map;
	struct
	{
		int16_t major;
		int16_t minor;
		int16_t patch;
	} supported_version;
} builtin_api = {0};



void build_gdscript_api(void)
{
	/* arena_t* type_workpad = workpad_checkout(2048 * 4 * 14); */
	// note: values that end in 0x1 are enums!
	int32_t hot_index = -1;
#include "builtin.gen"
	// turn all relative ptrs into absoute ptrs
	// OR--memcpy that shit out F A M 
	return;
}


void print_supported_version(void)
{
	printf("%d.%d.%d\n",
		   builtin_api.supported_version.major,
		   builtin_api.supported_version.minor,
		   builtin_api.supported_version.patch);
}
