#ifndef GID_MAP_H
#define GID_MAP_H

#include <stdint.h>
#include "../gdscript/gdscript_ids.h"


typedef struct gid_map_t
{
	char** keys;
	int8_t* lens;
	gdscript_id_t* values;
	int16_t cap;
	int16_t size;
} gid_map_t;

void gid_map_init(gid_map_t* map, uint8_t* allocation, const int16_t cap);

void gid_map_cache(gid_map_t* map, char* key, const int8_t len, const gdscript_id_t value);

gdscript_id_t gid_map_fetch(const gid_map_t* map, const char* key, const int8_t len);

void gid_map_print(const gid_map_t* map);


#endif /* End GID_MAP_H */
