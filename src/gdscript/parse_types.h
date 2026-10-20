#ifndef GDSCRIPT_PARSE_TYPES_H
#define GDSCRIPT_PARSE_TYPES_H

#include <stdint.h>
#include "../linked_list.h"

/* typedef struct parse_table_t */
/* { */
/* 	int32_t* token_indices; // doubles as ids for abstract thingies */
/* 	int16_t* depths; */
/* 	int32_t size; */
/* } parse_table_t; */

// TODO: stop using trees.  Impossible, I'm afraid :(
struct parse_node
{

	LINKED_TREE(struct parse_node);
	union
	{
		int32_t index;
		int32_t id;
	};
};


#endif /* End GDSCRIPT_PARSE_TYPES_H */
