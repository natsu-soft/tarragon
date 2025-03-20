#ifndef MEM_SYS_H
#define MEM_SYS_H

#include <stddef.h>
#include <stdio.h>

void* ms_mmap(void* addr, size_t len, int prot, int flags, int fildes, off_t offset);

int ms_munmap(void* addr, size_t len);

void* ms_map_file(char* filename, size_t len, int* out_filedes);

int ms_munmap_file(void* addr, size_t len, int fd);

void ms_swap(void* a, void* b, const size_t len);

void ms_buffer_swap(void* a, void* b, void* buf, const size_t len);

#endif /* End MEM_SYS_H */
