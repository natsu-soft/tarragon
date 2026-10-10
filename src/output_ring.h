#ifndef OUTPUT_RING_H
#define OUTPUT_RING_H

#include <stdint.h>
#include "strings.h"

void output_ring_init(void);
void output_ring_free(void);

int32_t output_ring_printf(char* format, ...);
int32_t output_ring_puts(const string* str);
int32_t output_ring_flush(void);

#endif /* End OUTPUT_RING_H */
