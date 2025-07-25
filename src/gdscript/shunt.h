#ifndef SHUNT_H
#define SHUNT_H

#include <stdint.h>

typedef struct token_table_t token_table_t;

const int32_t shunt(struct arena_t* arena, token_table_t* dest, const token_table_t* src, const int32_t n);

#endif /* End SHUNT_H */
