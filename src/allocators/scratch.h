#ifndef SCRATCH_H
#define SCRATCH_H

#include <stdint.h>


void* scratch_alloc(const int32_t size);

struct arena_t* scratch_arena(const int32_t size);

#endif /* End SCRATCH_H */
