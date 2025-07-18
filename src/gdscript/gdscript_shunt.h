#ifndef GDSCRIPT_SHUNT_H
#define GDSCRIPT_SHUNT_H

#include <stdint.h>

typedef struct token_table_t token_table_t;

int32_t gdscript_shunt(token_table_t* const dest, const token_table_t* src, const int32_t n);

#endif /* End GDSCRIPT_SHUNT_H */
