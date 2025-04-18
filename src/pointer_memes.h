#ifndef POINTER_MEMES_H
#define POINTER_MEMES_H

/* #include <stdint.h> */
#include <stddef.h>

static inline void point_relative(void** ptr)
{
	*ptr = (void*)((size_t)*ptr - (size_t)ptr);
}

static inline void point_absolute(void** ptr)
{
	*ptr = (void*)((size_t)*ptr + (size_t)ptr);
}

static inline void point_relative_offset(void** ptr, const void* base)
{
	*ptr = (void*)((size_t)*ptr - (size_t)base);
}

static inline void point_absolute_offset(void** ptr, const void* base)
{
	*ptr = (void*)((size_t)*ptr + (size_t)base);
}

#endif /* End POINTER_MEMES_H */
