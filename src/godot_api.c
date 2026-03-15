#include "godot_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "api_structs.h"
#include "allocators/scratch.h"
#include "allocators/workpad.h"
#include "hashmaps/api_map.h"
#include "godot_api.gen.h"


#define PASSERT(A, MSG, ...) if (!(A)) { fprintf(stderr, MSG, ##__VA_ARGS__); assert(A); }

#define VERBOSE_FETCH
#ifdef VERBOSE_FETCH
	#define FETCH_PRINT(MSG, ...) printf(MSG, ##__VA_ARGS__)
#else
	#define FETCH_PRINT(...)
#endif

static inline void cpystr(char* restrict dest, const char* restrict src, const int len)
{
	memcpy(dest, src, len);
	dest[len] = '\0';
}

const fetch_res_t api_fetch(char* key, int16_t len)
{
	api_map_t* map = api_map;
	fetch_res_t res = {0};
	if (key[0] != ' ')
	{
		int32_t i0 = 0;
		FETCH_PRINT("fetch: ");
		char* temp = alloca(len + 1);
		for (int32_t i = i0; i < len; ++i)
		{
			if (key[i] != '.') { continue; }
			else
			{
				diff_ptr_t diff_ptr = api_map_fetch(map, key + i0, i - i0);
				cpystr(temp, key + i0, i - i0);
				FETCH_PRINT("%s -> ", temp);
				PASSERT((diff_ptr.delta & 0x7) == API_CLASS, "'%s' should have fetched a class!\n", temp);
				diff_ptr.delta &= ~0x7;
				map = abs_diff(&diff_ptr, classes);
				i0 = i + 1;
			}
		}
		/* char* temp = alloca(len + 1); */
		cpystr(temp, key + i0, len - i0);
		FETCH_PRINT("%s (%d:", temp, len - i0);
		res.diff_ptr = api_map_fetch(map, key + i0, len - i0);
		res.tag = res.diff_ptr.delta & 0x07;
		res.diff_ptr.delta &= ~0x07;
		switch(res.tag)
		{
		case API_MEMBER:
			res.ptr = abs_diff(&res.diff_ptr, members);
			FETCH_PRINT("member)\n");
			break;
		case API_ENUM:
			res.ptr = abs_diff(&res.diff_ptr, enums);
			FETCH_PRINT("enum)\n");
			break;
		case API_FUNC:
			res.ptr = abs_diff(&res.diff_ptr, funcs);
			FETCH_PRINT("function)\n");
			break;
		case API_CLASS:
			res.ptr = abs_diff(&res.diff_ptr, classes);
			FETCH_PRINT("class)\n");
			break;
		case API_SIGNAL:
			res.ptr = abs_diff(&res.diff_ptr, signals);
			FETCH_PRINT("signal)\n");
			break;
		case API_CONSTANT:
			res.ptr = abs_diff(&res.diff_ptr, constants);
			FETCH_PRINT("constant)\n");
			break;
		case API_NATIVE_STRUCT:
			res.ptr = abs_diff(&res.diff_ptr, native_structs);
			FETCH_PRINT("struct)\n");
			break;
		default:
			FETCH_PRINT("ERR) -- DELTA: %#x\n", res.diff_ptr.delta);
			#ifdef VERBOSE_FETCH
			fflush(stdout);
			assert(0);
			#else
			assert(0 && "catastrophic fetch error -- enable VERBOSE_FETCH");
			#endif
		}
		fflush(stdout);
	}
	else
	{
		res.diff_ptr = api_map_fetch(map, key, len);
		res.tag = res.diff_ptr.delta & 0x07;
		res.diff_ptr.delta &= ~0x07;
		switch(res.tag)
		{
		case API_ENUM: res.ptr = abs_diff(&res.diff_ptr, enums); break;
		case API_CLASS: res.ptr = abs_diff(&res.diff_ptr, classes); break;
		default:
			assert(0 && "operation keys SHOULD return either an enum or a class!");
		}
	}
	res.diff_ptr.delta |= res.tag;
	return res;
}


int godot_api_test(int argc, char** argv)
{
	printf("testing api!\n");
	assert(api_map != NULL && "api_map is NULL!\n");
	printf("api_map is not NULL!\n");
	printf("api_binary: %0x\n", api_binary);
	printf("api_map: %0x\n", api_map);
	printf("api_size: %d\n", api_size);
	fflush(stdout);
	const uint64_t fresh_hash = api_map_hash(api_map, api_size);
	PASSERT((api_hash ^ fresh_hash) == 0x00, "hash mismatch! %#x != %#x\n", api_hash, fresh_hash);
	assert(api_hash == api_map_hash(api_map, api_size));
	printf("\tglobal api: ");
	api_map_audit(api_map, 0);
	printf("OK!\n");
	for (int32_t i = 0; i < num_classes; ++i)
	{
		const int32_t len = classes[i].name_len;
		char* temp = scratch_alloc(len + 1);
		memcpy(temp, abs_self(&classes[i].name_selfptr), len);
		temp[len] = '\0';
		printf("%s: ", temp);
		fflush(stdout);
		api_map_audit(classes + i, 0);
		printf("OK!\n", temp);
	}
	printf("SELF-AUDITS: OK!\n\n");
	WORKPAD* workpad = workpad_checkout(0);
	char** keys;
	int16_t* lens;
	int32_t size;
	api_map_dump_keys(&keys, &lens, &size, workpad, api_map);
	for (int32_t i = 0; i < size; ++i)
	{
		api_fetch(keys[i], lens[i]);
	}
	WORKPAD* namepad = workpad_checkout(512);
	char* query = namepad->allocation;
	for (int32_t i = 0; i < num_classes; ++i)
	{
		const int32_t name_len = classes[i].name_len;
		memcpy(query, abs_self(&classes[i].name_selfptr), name_len);
		query[name_len] = '.';
		char* tail = query + name_len + 1;
		api_map_dump_keys(&keys, &lens, &size, workpad, classes + i);
		for (int32_t j = 0; j < size; ++j)
		{
			memcpy(tail, keys[j], lens[j]);
			const int32_t query_len = name_len + lens[j] + 1;
			api_fetch(query, query_len);
		}
	}
	printf("FETCH TESTS: OK!\n");
	return 0;
}
