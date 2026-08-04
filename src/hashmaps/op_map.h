#ifndef OP_MAP_H
#define OP_MAP_H

#include <stdint.h>
#include "../gdscript/token_ids.h"

typedef struct op_map_t
{
	void** values;
	token_id_t* keys;
	int16_t cap;
	int16_t size;
} op_map_t;

void op_map_init(op_map_t* map, uint8_t* allocation, const int16_t cap);

void op_map_cache(op_map_t* map, const token_id_t key, const void* value);

void* op_map_fetch(op_map_t* map, const token_id_t key);


#endif /* End OP_MAP_H */
