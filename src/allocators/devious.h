#ifndef DEVIOUS_H
#define DEVIOUS_H

#include <stdint.h>

typedef struct arena_t arena_t;

arena_t* devious_checkout(void);

arena_t* devious_sized_checkout(const int32_t size);

void devious_checkin(arena_t* arena);

void* devious_alloc(const int32_t size);

void* devious_calloc(const int32_t size);


#ifndef DEVIOUS
static inline void _undevious(arena_t** a) { devious_checkin(*a); }
#define DEVIOUS __attribute__((__cleanup__(_undevious))) arena_t
#endif



#endif /* End DEVIOUS_H */
