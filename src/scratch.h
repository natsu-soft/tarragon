#ifndef SCRATCH_H
#define SCRATCH_H

#include <stddef.h>

void* scratch_alloc(const size_t bytes);

void* scratch_calloc(const size_t bytes);

struct arena_t* scratch_arena();

#endif /* End SCRATCH_H */
