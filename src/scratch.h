#ifndef SCRATCH_H
#define SCRATCH_H

#include <stddef.h>


void* scratch_alloc(size_t size);

void scratch_free();

void scratch_grow(size_t size);


#endif /* End SCRATCH_H */
