#ifndef GDSCRIPT_DIAGNOSTIC_TYPES_H
#define GDSCRIPT_DIAGNOSTIC_TYPES_H

#include <stdint.h>
#include "token_ids_types.h"


typedef struct diagnostic
{
	int32_t offset;
	int32_t len;
	token_id_t diagnostic_id;
} diagnostic;

#endif /* End GDSCRIPT_DIAGNOSTIC_TYPES_H */
