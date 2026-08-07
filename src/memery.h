#ifndef MEMERY_H
#define MEMERY_H

#include <stdint.h>
#include <stddef.h>
#include "memery_types.h"

virtual_buffer memalloc(size_t min_size);
void memfree(const virtual_buffer vb);
virtual_file map_file(const char* filename, size_t len);
void unmap_file(virtual_file virt_file);

void* ring_map(int32_t ring_size);
void ring_unmap(void* buffer, int32_t ring_size);


#endif /* End MEMERY_H */
