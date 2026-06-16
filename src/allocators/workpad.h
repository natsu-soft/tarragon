#ifndef WORKPAD_H
#define WORKPAD_H

#include <stdint.h>
struct workpad;
typedef struct arena_t arena_t;


// https://stackoverflow.com/questions/34574933/a-good-and-idiomatic-way-to-use-gcc-and-clang-attribute-cleanup-and-point
// memory leaks are a skill issue
// so are double frees

arena_t* workpad_checkout(const int32_t size);

void workpad_checkin(struct workpad* workpad);

void workpad_checkin_all(void);

// I dont even want this anymore tbqh
#ifndef WORKPAD
static inline void _cleanup_workpad(arena_t** p) { workpad_checkin((struct workpad*)*p); }
#define WORKPAD __attribute__((__cleanup__(_cleanup_workpad))) arena_t
#endif

#endif /* End WORKPAD_H */
