#include "smol.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MIN(A, B) (A < B ? A : B)


// https://research.cs.vt.edu/AVresearch/hashing/strings.php
// trash hash func tbqh
static const int32_t hash(char* str, const int32_t len, const int32_t m)
{
	assert(len > 0 && "you wanna hash a zero-len string?  go fuck yourself.");
	int32_t sum = 0;
	for (int32_t i = 0; i < len; ++i)
	{
		sum += str[i];
	}
	return sum % m;
}

// Rule of Thumb: you need 4x the space for the amount of items you expect to cache in the hashmap
void smol_init(smol_t* smol, int32_t expected_entries)
{
	map_entry_t* entries = calloc(4 * expected_entries, sizeof *entries);
	assert(entries != NULL && "smol_init failed to malloc");
	smol->entries = entries;
	smol->cap = 4 * expected_entries * sizeof *entries;
}

void smol_cap(smol_t* smol, int32_t expected_entries)
{
	smol->cap = 4 * expected_entries * sizeof *smol->entries;
}


void smol_cache(smol_t* smol, char* key, int32_t len, void* ptr)
{
	assert(smol->size < smol->cap && "smol out of space!");
	const int32_t cap = smol->cap;
	int32_t index = hash(key, len, cap);
	map_entry_t* entries = smol->entries;
	for (int32_t i = 0; i < smol->cap; ++i)
	{
		if (entries[index].len == 0)
		{
			break;
		}
		else
		{
			index = (index + 1) % cap;
		}
	}
	entries[index] = (map_entry_t)
	{
		.key = key,
		.len = len,
		.ptr = ptr
	};
	++smol->size;
}

void* smol_fetch(smol_t* smol, char* key, int32_t len)
{
	const int32_t cap = smol->cap;
	const int32_t i0 = hash(key, len, cap);
	const map_entry_t* entries = smol->entries;
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t index = (i0 + i) % cap;
		const int32_t l = MIN(entries[index].len, len);
		if (l > 0 && strncmp(key, entries[index].key, l) == 0)
		{
			return entries[index].ptr;
		}
		else if (l == 0)
		{
			return NULL;
		}
	}
	return NULL;
}
