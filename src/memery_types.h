#ifndef MEMERY_TYPES_H
#define MEMERY_TYPES_H

#include <stddef.h>
#include <stdint.h>

#define KB 1024
#define MB 1048576

enum memery_enums
{
	ENTIRE_FILE = -1,
};

struct virtfile_handle
{
	uint64_t opaque;
};

typedef struct virtual_buffer
{
	void* addr;
	size_t size;
} virtual_buffer;

typedef struct virtual_file
{
	virtual_buffer virt_buffer;
	struct virtfile_handle handle;
} virtual_file;

#endif /* End MEMERY_TYPES_H */
