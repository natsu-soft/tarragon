#include "mem_sys.h"
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include "error_macros.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <memoryapi.h>
#else
#include <sys/mman.h>
#import <sys/stat.h>
#import <fcntl.h>
#endif

// TODO: TEST WINDOWS

void* ms_mmap(void* addr, size_t len)
{
#ifdef _WIN32
	// see: https://learn.microsoft.com/en-us/windows/win32/memory/allocating-virtual-memory
	// eg: https://learn.microsoft.com/en-us/windows/win32/Memory/reserving-and-committing-memory
	void* ptr = VirtualAlloc(NULL, len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	assertf(ptr != NULL, "VirtualAlloc failed with error %lu\n", GetLastError());
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
	return VirtualFree(addr, 0, MEM_RELEASE);
#else
	return munmap(addr, len);
#endif
}


void* ms_map_file(const char* filename, size_t len, handle_t* out_handle)
{
#ifdef _WIN32
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
	void* res = MapViewOfFile(file_map, FILE_MAP_ALL_ACCESS, 0, 0, len);
	assertf(res != NULL, "MapViewOfFile of file %s failed with error %lu\n", filename, GetLastError());
	*out_handle = file_map;
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
	int res = munmap(addr, len);
	assert(close(handle) == 0);
	return res;
#endif
}

// DOES IT SPARK JOY?
void* ms_map_file2(char* filename, size_t len, filemap_t* out_filemap)
{
#ifdef _WIN32
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
	void* res = MapViewOfFile(file_map, FILE_MAP_ALL_ACCESS, 0, 0, len);
	assertf(res != NULL, "MapViewOfFile of file %s failed with error %lu\n", filename, GetLastError());
	out_filemap->ptr = res;
	out_filemap->len = len;
	out_filemap->handle = file_map;
	return res;
#else
	int filedes = open(filename, O_RDWR | O_CREAT | O_APPEND, 0400 | 0200);
	assertf(filedes != -1, "failed to get fd in ms_map_file().  Check the perms!\n");
	assertf(ftruncate(filedes, len) != -1, "failed to truncate file %s to desired length: %zu\n", filename, len);
	void* res = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, filedes, 0);
	out_filemap->ptr = res;
	out_filemap->len = len;
	out_filemap->handle = filedes;
	return res;
#endif
}

// DOES IT SPARK JOY?
int ms_munmap_file2(filemap_t* filemap)
{
#ifdef _WIN32
	BOOL res = UnmapViewOfFile(filemap->ptr);
	res &= CloseHandle(filemap->handle);
	return 1 - res;
#else
	int res = munmap(filemap->ptr, filemap->len);
	close(filemap->handle);
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
	handle_t file_handle;
	char* ptr = ms_map_file("map.txt", 1024, &file_handle);
	for(uint16_t i = 0; i < 1024; ++i)
	{
		printf("%c", ptr[i]);
	}
	const char* string = "BASED BASED BASED";
	memcpy(ptr, string, strlen(string));
	return ms_munmap_file(ptr, 1024, file_handle);
}
