#ifndef MEMERY_TYPES_H
#define MEMERY_TYPES_H

#include <stddef.h>

typedef struct virtual_buffer
{
	void* addr;
	size_t size;
} virtual_buffer;

#define KB 1024
#define MB 1048576

#endif /* End MEMERY_TYPES_H */
