#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdint.h>

typedef struct token_table_t token_table_t;


typedef enum __attribute__((packed)) diagnostics_output_t
{
	DIAGNOSTICS_STDOUT = 0x01,
	DIANGOSTICS_JSONRPC = 0x02,
} diagnostics_output_t;

void diagnostic_note(const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len);
void diagnostic_warn(const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len);
void diagnostic_error(const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len);

const int32_t diagnostics_pre_analysis(token_table_t* const table, const int32_t src_len, const char* path);

#endif /* End DIAGNOSTICS_H */
