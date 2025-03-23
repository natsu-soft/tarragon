#include "mem_sys.h"
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include "error_macros.h"

#ifdef _WIN32
// TODO: VirtualAlloc support
// #include <memoryapi.h>
#else
#include <sys/mman.h>
// for open()
#import <sys/stat.h>
#import <fcntl.h>
#endif

#ifdef _WIN32
void* ms_mmap(void* addr, size_t len, int prot, int flags, int fildes, off_t offset)
{
	return NULL;
}
#else
void* ms_mmap(void* addr, size_t len, int prot, int flags, int fildes, off_t offset)
{
	void* ptr = mmap(addr, len, prot, flags, fildes, offset);
	assertf(ptr != MAP_FAILED, "mmap() failed!\n");
	return ptr;
}
#endif

#ifdef _WIN32
int ms_munmap(void* addr, size_t len)
{
	return 0;
}
#else
int ms_munmap(void* addr, size_t len)
{
	return munmap(addr, len);
}
#endif


void* ms_map_file(char* filename, size_t len, int* out_filedes)
{
	#ifdef _WIN32
	return NULL;
	#else
	int filedes = open(filename, O_RDWR | O_CREAT | O_APPEND, 0400 | 0200);
	assertf(filedes != -1, "failed to get fd in ms_map_file().  Check the perms!\n");
	assertf(ftruncate(filedes, len) != -1, "failed to truncate file %s to desired length: %zu\n", filename, len);
	*out_filedes = filedes;
	return ms_mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, filedes, 0);
	#endif
}

int ms_munmap_file(void* addr, size_t len, int fd)
{
	#ifdef _WIN32
	return 0;
	#else
	int res = ms_munmap(addr, len);
	assert(close(fd) == 0);
	return res;
	#endif
}

void ms_swap(void* a, void* b, const size_t len)
{
	uint8_t buf[len];
	memcpy(buf, a, len);
	memcpy(b, a, len);
	memcpy(a, buf, len);
}

void ms_buffer_swap(void* a, void* b, void* buf, const size_t len)
{
	memcpy(buf, a, len);
	memcpy(b, a, len);
	memcpy(a, buf, len);
}

int mem_sys_test(int argc, char** argv)
{
	int fd;
	char* ptr = ms_map_file("map.txt", 1024, &fd);
	char* string = "Wow wao wao wao wao WAOW";
	memcpy(ptr, string, strlen(string));
	ms_munmap_file(ptr, 1024, fd);
	return 0;
}
