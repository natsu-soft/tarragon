#ifndef MEMERY_TYPES_H
#define MEMERY_TYPES_H

#include <stdint.h>

#define PAGE_SIZE 4096
#define KB 1024
#define MB 1048576
#define GB 1073741824

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
	int32_t size;
} virtual_buffer;

typedef struct virtual_file
{
	virtual_buffer virt_buffer;
	struct virtfile_handle handle;
} virtual_file;

// the virt buffer pun is intentional
typedef struct ring_buffer
{
	void* addr;
	int32_t size;
	int fd; // note this gets closed automagically once all references dropped SO YOU HAVE TO KEEP THE FD
	int32_t head;
	int32_t tail;
} ring_buffer;

#endif /* End MEMERY_TYPES_H */
