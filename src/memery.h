#ifndef MEMERY_H
#define MEMERY_H

#include <stdint.h>
#include "memery_types.h"

virtual_buffer memalloc(int32_t min_size);
void memfree(virtual_buffer* vb);
virtual_file map_file(const char* filename, int32_t len);
void unmap_file(virtual_file virt_file);

int32_t ring_map(ring_buffer* rb, int32_t min_size, char* anon_filename);
void ring_unmap(ring_buffer* rb);


#endif /* End MEMERY_H */
