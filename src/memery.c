#include "memery.h"
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#if _WIN32
#error OH JESUS IMPLEMENT THIS FOR WINDOWS
#else
#include <sys/mman.h>
#endif

#warning if u were a dweeb ud check page size per system
#define PAGE_SIZE 4096
#define PAGE_MASK 4095


virtual_buffer memalloc(size_t min_size)
{
	// too lazy for peak bitmemery
	size_t size = PAGE_SIZE * (min_size / PAGE_SIZE) + (min_size & PAGE_MASK ? PAGE_SIZE : 0);
	void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1 , 0);
	if (p == MAP_FAILED)
	{
		fprintf(stderr, "ERRNO %d: %s\n", errno, strerror(errno));
		fflush(stderr);
		assert(0 && "\tmmap() failed in memalloc()\n");
	}
	return (virtual_buffer)
	{
		.addr = p,
		.size = size
	};
}

void memfree(const virtual_buffer vb)
{
	assert(munmap(vb.addr, vb.size) == 0);
}


void* ring_map(int32_t ring_size)
{
	return 0x00;
}

void ring_unmap(void* buffer, int32_t ring_size)
{
}
