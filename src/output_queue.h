#ifndef OUTPUT_QUEUE_H
#define OUTPUT_QUEUE_H

#include <stdint.h>
#include "strings.h"

void output_queue_init(void);
void output_queue_free(void);

int32_t output_queue_printf(char* format, ...);
int32_t output_queue_puts(const string* str);
int32_t output_queue_flush(void);

#endif /* End OUTPUT_QUEUE_H */
