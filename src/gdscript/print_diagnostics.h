#ifndef PRINT_DIAGNOSTICS_H
#define PRINT_DIAGNOSTICS_H

#include <stdint.h>
#include "../strings.h"

// NOTE: string* dest assumes sufficient size
void diagnostic_note(string* dest, const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len);
void diagnostic_warn(string* dest, const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len);
void diagnostic_error(string* dest, const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len);

void print_note(const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len);
void print_warn(const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len);
void print_error(const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len);


#endif /* End PRINT_DIAGNOSTICS_H */
