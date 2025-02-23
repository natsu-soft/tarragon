#ifndef PROGRAM_MEMORY_H
#define PROGRAM_MEMORY_H

#include <stddef.h>

#define PAGESIZE 4096

void pm_init(size_t pages);

void pm_deinit();

void* pm_malloc(size_t bytes);

void pm_free(void* addr);

#endif /* End PROGRAM_MEMORY_H */
