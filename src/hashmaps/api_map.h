#ifndef API_MAP_H
#define API_MAP_H

#include <stdint.h>
#include "../relative_ptrs.h"
#include "../gdscript/gdscript_ids.h"


typedef struct api_map_t
{
	self_ptr_t keys_selfptrs;
	self_ptr_t values_diffptrs;
	self_ptr_t lens_i16;
	int32_t cap;
	int32_t size;
} api_map_t;

typedef struct arena_t arena_t;


void api_map_back(api_map_t* map, arena_t* backing_arena, const int32_t cap);
diff_ptr_t* api_map_cache(api_map_t* map, const char* key, const int32_t len,  const diff_ptr_t diff_ptr);
diff_ptr_t api_map_fetch(const api_map_t* map, const char* key, const int32_t len);
void api_map_audit(const api_map_t* map);

const uint64_t api_map_hash(const uint8_t* str, const int32_t len);


#endif /* End API_MAP_H */
