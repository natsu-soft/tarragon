#ifndef SMOL_H
#define SMOL_H

#include <stdint.h>

// really only for SMALL hashmaps of strings tbqh

typedef struct map_entry_t
{
	char** key;
	void* ptr;
	int32_t len;
} map_entry_t;

typedef struct smol_t
{
	map_entry_t* entries;
	int32_t cap;
	int32_t size;
} smol_t;

void smol_init(smol_t* smol, int32_t expected_entries);

void smol_cap(smol_t* smol, int32_t expected_entries);

void smol_cache(smol_t* smol, char* key, int32_t len, void* ptr);

void* smol_fetch(smol_t* smol, char* key, int32_t len);

#endif /* End SMOL_H */
