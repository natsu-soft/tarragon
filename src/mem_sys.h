#ifndef MEM_SYS_H
#define MEM_SYS_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#if _WIN32
typedef void* handle_t; // jfc microsoft, a HANDLE
#else
typedef int ms_handle_t; // a filedesc for *NIX systems
#endif

// DOES IT SPARK JOY?  No.  Not really tbqh
typedef struct filemap_t
{
	void* ptr;
	size_t len;
	ms_handle_t handle;
} filemap_t;

typedef struct arena_t arena_t;

/* void* ms_mmap(void* addr, size_t len, int prot, int flags, int fildes, off_t offset); */
void* ms_mmap(void* addr, size_t len);

int ms_munmap(void* addr, size_t len);

void* ms_map_entire_file(const char* filename, int32_t* out_len, ms_handle_t* out_handle);

void* ms_map_file(const char* filename, size_t len, ms_handle_t* out_handle);

int ms_munmap_file(void* addr, size_t len, ms_handle_t handle);

void* ms_map_file2(char* filename, size_t len, filemap_t* out_filemap);

int ms_munmap_file2(filemap_t* filemap);

void ms_swap(void* a, void* b, const size_t len);

void ms_buffer_swap(void* a, void* b, void* buf, const size_t len);

void ms_buffer_swap(void* a, void* b, void* buf, const size_t len);

// just use memmove lmao
/* size_t ms_resize_array(void* array, const unsigned int dest, const unsigned int src, const size_t item_size, const size_t n); */

#endif /* End MEM_SYS_H */
