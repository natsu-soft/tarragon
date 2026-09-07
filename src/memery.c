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
#include <sys/mman.h>
#endif
#include "error_macros.h"

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

#ifdef _WIN32
virtual_file map_file(const char* filename, size_t len)
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
virtual_file map_file(const char* filename, size_t len)
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
		.virt_buffer.addr = ptr,
		.virt_buffer.size = len,
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
void* ring_map(int32_t ring_size)
{
	return 0x00;
}

void ring_unmap(void* buffer, int32_t ring_size)
{
}
