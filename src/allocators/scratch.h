#ifndef SCRATCH_H
#define SCRATCH_H

#include <stdint.h>
#include "arena.h"

// https://stackoverflow.com/questions/34574933/a-good-and-idiomatic-way-to-use-gcc-and-clang-attribute-cleanup-and-point
// memory leaks are a skill issue
// so are double frees

arena_t* scratch_checkout(void);

arena_t* scratch_checkout_size(const uint32_t size);

void scratch_checkin(const arena_t* scratch);

void scratch_checkin_all(void);

void* scratch_alloc(const uint32_t size);

void* scratch_calloc(const uint32_t size);

void scratch_audit(void);


#ifndef SCRATCH
static inline void _cleanup_scratch(arena_t** p) { scratch_checkin(*p); }
#define SCRATCH __attribute__((__cleanup__(_cleanup_scratch))) arena_t
#endif

#endif /* End SCRATCH_H */
