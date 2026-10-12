#ifndef STDOUT_RING_H
#define STDOUT_RING_H

#include <stdint.h>
#include "strings.h"

void stdout_ring_init(void);
void stdout_ring_free(void);

int32_t stdout_ring_printf(char* format, ...);
int32_t stdout_ring_puts(const string* str);
int32_t stdout_ring_flush(void);

#endif /* End STDOUT_RING_H */
