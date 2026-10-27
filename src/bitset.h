#ifndef BITSET_H
#define BITSET_H

#include <stdint.h>
#include "bitset_types.h"

void bitset_add(struct bitset* set, const int32_t value);
uint8_t bitset_has(struct bitset* set, const int32_t value);


#endif /* End BITSET_H */
