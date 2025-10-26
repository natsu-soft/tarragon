#ifndef RELATIVE_PTRS_H
#define RELATIVE_PTRS_H

#include <stdint.h>
#include <stddef.h>

/* typedef int32_t self_t; */
/* typedef int32_t diff_t; */


// somewhat stricter typing pls tyvm
// NOTE: you can pass & return by value since they're both small enough!

typedef struct self_ptr_t
{
	int32_t delta;
} self_ptr_t;
typedef struct diff_ptr_t
{
	int32_t delta;
} diff_ptr_t;


static inline void self_relate(self_ptr_t* self, const void* ptr)
{
	self->delta = (int32_t)((size_t)ptr - (size_t)self);
}

static inline const diff_ptr_t diff_relate(const void* ptr, const void* base)
{
	return (diff_ptr_t){ .delta = (int32_t)((size_t)ptr - (size_t)base) };
}

static inline void* abs_self(const self_ptr_t* self)
{
	return (uint8_t*)self + self->delta;
}

static inline void* abs_diff(const diff_ptr_t* diff, const void* base)
{
	return (uint8_t*)base + diff->delta;
}

#endif /* End RELATIVE_PTRS_H */
