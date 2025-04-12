#ifndef MEM_SYS_H
#define MEM_SYS_H

#include <stddef.h>
#include <stdio.h>

#if _WIN32
typedef void* handle_t;
#else
typedef int handle_t;
#endif

/* void* ms_mmap(void* addr, size_t len, int prot, int flags, int fildes, off_t offset); */
void* ms_mmap(void* addr, size_t len);

int ms_munmap(void* addr, size_t len);

void* ms_map_file(char* filename, size_t len, handle_t* out_handle);

int ms_munmap_file(void* addr, size_t len, handle_t handle);

void ms_swap(void* a, void* b, const size_t len);

void ms_buffer_swap(void* a, void* b, void* buf, const size_t len);

void ms_buffer_swap(void* a, void* b, void* buf, const size_t len);

// just use memmove lmao
/* size_t ms_resize_array(void* array, const unsigned int dest, const unsigned int src, const size_t item_size, const size_t n); */

#endif /* End MEM_SYS_H */
