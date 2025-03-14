#include "mem_sys.h"
#include <string.h>
#include <unistd.h>
#include <assert.h>
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
	int filedes = open(filename, O_RDWR | O_CREAT | O_APPEND, 0400 | 0200);
	assertf(filedes != -1, "failed to get fd in ms_map_file().  Check the perms!\n");
	assertf(ftruncate(filedes, len) != -1, "failed to truncate file %s to desired length: %zu\n", filename, len);
	*out_filedes = filedes;
	return ms_mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, filedes, 0);
}

int ms_munmap_file(void* addr, size_t len, int fd)
{
	int res = ms_munmap(addr, len);
	assert(close(fd) == 0);
	return res;
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
