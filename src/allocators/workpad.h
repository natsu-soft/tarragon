#ifndef WORKPAD_H
#define WORKPAD_H

#include <stdint.h>
#include "arena.h"

typedef struct workpad_t
{
	arena_t arena;
	struct workpad_t* next;
} workpad_t;

// https://stackoverflow.com/questions/34574933/a-good-and-idiomatic-way-to-use-gcc-and-clang-attribute-cleanup-and-point
// memory leaks are a skill issue
// so are double frees

arena_t* workpad_checkout(const int32_t size);

void workpad_checkin(workpad_t* workpad);

void workpad_checkin_all(void);

#ifndef WORKPAD
static inline void _cleanup_workpad(arena_t** p) { workpad_checkin((workpad_t*)*p); }
#define WORKPAD __attribute__((__cleanup__(_cleanup_workpad))) arena_t
#endif

#endif /* End WORKPAD_H */
