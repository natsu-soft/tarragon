#include "mem_sys.h"
#include <minwindef.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <winnt.h>
#include "error_macros.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winbase.h>
#include <handleapi.h>
#include <memoryapi.h>
#else
#include <sys/mman.h>
// for open()
#import <sys/stat.h>
#import <fcntl.h>
#endif

// TODO: TEST WINDOWS

void* ms_mmap(void* addr, size_t len);
{
#ifdef _WIN32
	return NULL;
#else
	void* ptr = mmap(addr, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	assertf(ptr != MAP_FAILED, "mmap() failed!\n");
	return ptr;
#endif
}

int ms_munmap(void* addr, size_t len)
{
#ifdef _WIN32
	return NULL;
#else
	return munmap(addr, len);
#endif
}


void* ms_map_file(char* filename, size_t len, handle_t* out_handle)
{
#ifdef _WIN32
	HANDLE h_file = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, filename);
	assertf(h_file != NULL, "OpenFileMapping to file %s failed with error %lu\n", filename, GetLastError());
	*out_handle = h_file;
	LPVOID res = MapViewOfFile(h_file, FILE_MAP_READ | FILE_MAP_WRITE, 0x00, 0x00, len);
	assertf(res != NULL, "MapViewOfFile failed with error %lu\n", GetLastError());
	return res;
#else
	int filedes = open(filename, O_RDWR | O_CREAT | O_APPEND, 0400 | 0200);
	assertf(filedes != -1, "failed to get fd in ms_map_file().  Check the perms!\n");
	assertf(ftruncate(filedes, len) != -1, "failed to truncate file %s to desired length: %zu\n", filename, len);
	*out_handle = filedes;
	return mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, filedes, 0);
#endif
}

int ms_munmap_file(void* addr, size_t len, handle_t handle)
{
#ifdef _WIN32
	BOOL res = UnmapViewOfFile(addr);
	res &= CloseHandle(handle);
	return 1 - res;
#else
	int res = ms_munmap(addr, len);
	assert(close(handle) == 0);
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
	memcpy(buf, b, len);
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
