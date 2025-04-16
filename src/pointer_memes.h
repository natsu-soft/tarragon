#ifndef POINTER_MEMES_H
#define POINTER_MEMES_H

#include <stdint.h>

static inline void point_relative(void** ptr)
{
	*ptr = (void*)((int64_t)*ptr - (int64_t)ptr);
}

static inline void point_absolute(void** ptr)
{
	*ptr = (void*)((int64_t)*ptr + (int64_t)ptr);
}

static inline void point_relative_to(void** ptr, const void* base)
{
	*ptr = (void*)((int64_t)*ptr - (int64_t)base);
}

static inline void point_absolute_to(void** ptr, const void* base)
{
	*ptr = (void*)((int64_t)*ptr + (int64_t)base);
}

#endif /* End POINTER_MEMES_H */
