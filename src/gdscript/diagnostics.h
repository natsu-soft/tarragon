#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdint.h>
#include "diagnostics_types.h"

void diagnostics_init(void);

void diagnostics_add(const enum diagnostic_msg msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len);

void flush_diagnostics(void);

#endif /* End DIAGNOSTICS_H */
