#ifndef TOKEN_ID_MAP_H
#define TOKEN_ID_MAP_H

#include <stdint.h>
#include "token_ids_types.h"

typedef struct token_id_map_t
{
	char** keys;
	int8_t* lens;
	token_id_t* values;
	int16_t cap;
	int16_t size;
} token_id_map_t;

void token_id_map_init(token_id_map_t* map, uint8_t* allocation, const int16_t cap);

void token_id_map_cache(token_id_map_t* map, char* key, int8_t len, token_id_t value);

token_id_t token_id_map_fetch(const token_id_map_t* map, const char* key, const int8_t len);

void token_id_map_print(const token_id_map_t* map);


#endif /* End TOKEN_ID_MAP_H */
