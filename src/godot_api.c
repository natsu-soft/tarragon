#include "godot_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "api_structs.h"
#include "allocators/scratch.h"
#include "hashmaps/api_map.h"


#define PASSERT(A, MSG, ...) if (!(A)) { fprintf(stderr, MSG, ##__VA_ARGS__); assert(A); }

#include "godot_api.gen"

static api_map_t* api_map = api_binary;

// this a cluster fuck of conditionals.
struct api_fetch_res api_fetch(char* key, int16_t len)
{
	if (len > 0 && key[0] == ' ')
	{
		diff_ptr_t diff_ptr = api_map_fetch(api_map, key, len);
		return (struct api_fetch_res)
		{
			.ptr = abs_diff(&diff_ptr, (diff_ptr.delta & 0x7) == API_CLASS ? classes : enums),
			.tag = diff_ptr.delta & 0x7
		};
	}
	api_map_t* map = api_map;
	if (strncmp("enum::", key, 6) == 0)
	{
		key += 6;
		len -= 6;
	}
	else if (strncmp("bitfield::", key, 10) == 0)
	{
		key += 10;
		len -= 10;
	}
	if (strncmp("Variant.", key, 8) != 0)
	{
		int32_t i0 = 0;
		/* printf(" > "); */
		for (int32_t i = i0; i < len; ++i)
		{
			if (key[i] != '.') { continue; }
			else
			{
				/* class_t* class = (class_t*)((size_t)api_map_fetch(map, k + i0, i - i0) & 0xf); */
				diff_ptr_t diff = api_map_fetch(map, key + i0, i - i0);
				char* temp = alloca(len + 1);
				memcpy(temp, key, len);
				temp[len] = '\0';
				PASSERT((diff.delta & 0x7) == API_CLASS, "ERROR: '%s' should have fetched a class!\n", temp)
				/* assert((diff.delta & 0x7) == API_CLASS && "really should have fetched a class!"); */
				diff.delta &= ~0x7;
				map = abs_diff(&diff, classes);
				i0 = i + 1;
			}
		}
		const diff_ptr_t res = api_map_fetch(map, key + i0, len - i0);
		switch (res.delta & 0x7)
		{
		case API_MEMBER:
			return (struct api_fetch_res){ .ptr = abs_diff(&res, members), .tag = API_MEMBER };
		case API_ENUM:
			return (struct api_fetch_res){ .ptr = abs_diff(&res, enums), .tag = API_ENUM };
		case API_FUNC:
			return (struct api_fetch_res){ .ptr = abs_diff(&res, funcs), .tag = API_FUNC };
		case API_CLASS:
			return (struct api_fetch_res){ .ptr = abs_diff(&res, classes), .tag = API_CLASS };
		case API_SIGNAL:
			return (struct api_fetch_res){ .ptr = abs_diff(&res, signals), .tag = API_SIGNAL };
		case API_CONSTANT:
			return (struct api_fetch_res){ .ptr = abs_diff(&res, constants), .tag = API_CONSTANT };
		}
		return (struct api_fetch_res){ .ptr = NULL, .tag = API_NONE };
	}
	else if (strncmp("Variant.Type", key, 12) == 0 || strncmp("Variant.Operator", key, 16) == 0)
	{
		diff_ptr_t res = api_map_fetch(map, key, len);
		return (struct api_fetch_res){ .ptr = abs_diff(&res, enums), .tag = API_ENUM };
	}
	else
	{
		return (struct api_fetch_res){ .ptr = NULL, .tag = API_NONE };
	}
}


int godot_api_test(int argc, char** argv)
{
	printf("auditing api!\n");
	printf("\tglobal: ");
	api_map_audit(api_map);
	printf("OK!\n");
	for (int32_t i = 0; i < num_classes; ++i)
	{
		char* name = abs_self(&classes[i].name_selfptr);
		const int32_t len = classes[i].name_len;
		char* temp = scratch_alloc(len + 1);
		memcpy(temp, name, len);
		name[len] = '\0';
		printf("\t %s: ", temp);
		fflush(stdout);
		api_map_audit(classes + i);
		printf("OK!\n");
	}
	return 0;
	/* self_ptr_t* keys_selfptrs = abs_self(&api_map->keys_selfptrs); */
	/* int16_t* lens = abs_self(&api_map->lens_i16); */
	/* diff_ptr_t* value_diffptrs = abs_self(&api_map->values_diffptrs); */
	/* for (int32_t i = 0; i < api_map->cap; ++i) */
	/* { */
	/* 	const int16_t len0 = lens[i]; */
	/* 	if (len0 > 0) */
	/* 	{ */
	/* 		char* key = abs_self(keys_selfptrs + i); */
	/* 		struct api_fetch_res res = api_fetch(key, len0); */
	/* 		assert(res.tag != API_NONE && "fetched a bad result!\n"); */
	/* 		if (key[0] != ' ' && (res.tag & 0x7) == API_CLASS) */
	/* 		{ */
	/* 			class_t* class = res.ptr; */
	/* 			api_map_audit(class); */
	/* 			self_ptr_t* class_keys_selfptrs = abs_self(&class->keys_selfptrs); */
	/* 			int16_t* class_lens = abs_self(&class->lens_i16); */
	/* 			for (int32_t j = 0; j < class->cap; ++j) */
	/* 			{ */
	/* 				if (class_lens[j] > 0) */
	/* 				{ */
	/* 					const int16_t len = len0 + class_lens[j] + 1; */
	/* 					char* temp = scratch_alloc(len + 1); */
	/* 					memcpy(temp, key, len0); */
	/* 					temp[len0] = '.'; */
	/* 					memcpy(temp + lens[i] + 1, abs_self(class_keys_selfptrs + j), class_lens[j]); */
	/* 					temp[len] = '\0'; */
	/* 					printf("> %s (%d:%d)\n", temp, len0, len); */
	/* 					struct api_fetch_res res = api_fetch(key, lens[i]); */
	/* 					assert(res.tag != API_NONE && "fetched a bad result!\n"); */
	/* 				} */
	/* 			} */
	/* 		} */
	/* 	} */
	/* } */
	/* return 0; */
}
