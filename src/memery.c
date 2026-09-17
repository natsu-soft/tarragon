#include "memery.h"
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#if _WIN32
#error OH JESUS IMPLEMENT THIS FOR WINDOWS
#else
#include <fcntl.h>
#include <sys/stat.h>
#define _GNU_SOURCE
// gotta define GNU_SOURCE for memfd_create for ring maps
#include <sys/mman.h>
#endif
#include "error_macros.h"

#warning if u were a dweeb ud check page size per system
#define PAGE_MASK 4095


virtual_buffer memalloc(int32_t min_size)
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

void memfree(virtual_buffer* vb)
{
	assert(munmap(vb->addr, vb->size) == 0);
}

#ifdef _WIN32
virtual_file map_file(const char* filename, int32_t len)
{
	// see: https://learn.microsoft.com/en-us/windows/win32/memory/file-mapping
	HANDLE h_file = CreateFileA(filename,
								GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								NULL);
	assertf(h_file != INVALID_HANDLE_VALUE, "CreateFileA to file %s failed with error %lu\n", filename, GetLastError());
	HANDLE file_map = CreateFileMappingA(h_file, NULL, PAGE_READWRITE, 0, len, NULL);
	assertf(file_map != NULL, "CreateFileMappingA to file %s failed with error %lu\n", filename, GetLastError());
	// see: https://learn.microsoft.com/en-us/windows/win32/memory/creating-a-file-view
	void* buffer = MapViewOfFile(file_map, FILE_MAP_ALL_ACCESS, 0, 0, len);
	assertf(buffer != NULL, "MapViewOfFile of file %s failed with error %lu\n", filename, GetLastError());
	virtual_file res =
	{ 0

	};
	return res;
}
#else

virtual_file map_file(const char* filename, int32_t len)
{
	int fd = open(filename, O_RDWR | O_CREAT | O_APPEND, 0400 | 0200);
	assertf(fd != -1, "failed to get fd in ms_map_file().  Check the perms!\n");
	size_t size = len;
	if (len == ENTIRE_FILE)
	{
		len = lseek(fd, 0, SEEK_END);
		size = PAGE_SIZE * (len / PAGE_SIZE) + (len & PAGE_MASK ? PAGE_SIZE : 0) + PAGE_SIZE;
	}
	else
	{
		size = PAGE_SIZE * (len / PAGE_SIZE) + (len & PAGE_MASK ? PAGE_SIZE : 0) + PAGE_SIZE;
		assertf(ftruncate(fd, len) != -1, "failed to truncate file %s to desired length: %zu\n", filename, len);
	}
	void* ptr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	virtual_file res =
	{
		.virt_buffer =
		{
			.addr = ptr,
			.size = len
		},
		.handle = fd
	};
	return res;
}
#endif

void unmap_file(virtual_file virt_file)
{
	/* TODO */
}


// https://lo.calho.st/posts/black-magic-buffer/
// https://fgiesen.wordpress.com/2012/07/21/the-magic-ring-buffer/
int32_t ring_map(ring_buffer* rb, int32_t min_size, char* anon_filename)
{
	// TODO errchecking
	const int32_t size = PAGE_SIZE * (min_size / PAGE_SIZE) + (min_size & PAGE_MASK ? PAGE_SIZE : 0);
	rb->fd = memfd_create(anon_filename, 0x00);
	ftruncate(rb->fd, size);
	rb->addr = mmap(NULL,  2 * size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	void* p = mmap(rb->addr, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, rb->fd, 0);
	p = mmap((uint8_t*)rb->addr + size, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, rb->fd, 0);
	rb->size = size;
	return size;
}

/* The munmap() function shall remove any mappings for those entire pages containing
   any part of the address space of the process starting at addr  and  continuing
   for  len bytes. Further references to these pages shall result in the generation
   of a SIGSEGV signal to the process.  If there are no mappings in the specified
   address range, then munmap() has no effect.
*/
void ring_unmap(ring_buffer* rb)
{
	munmap(rb->addr, 2 * rb->size);
	close(rb->fd);
}
