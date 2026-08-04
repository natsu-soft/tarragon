#ifndef GDSCRIPT_PARSE_TYPES_H
#define GDSCRIPT_PARSE_TYPES_H

#include <stdint.h>

typedef struct parse_table_t
{
	int32_t* token_indices; // doubles as ids for abstract thingies
	int16_t* depths;
	int32_t size;
} parse_table_t;


#endif /* End GDSCRIPT_PARSE_TYPES_H */
