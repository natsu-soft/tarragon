#include "tokens.h"
/* #include <stdlib.h> */

int32_t token_line(const token_table_t* table, const int32_t index)
{
	int16_t line = 1;
	const int32_t offset = table->offsets[index];
	const char* src = table->src;
	for (int32_t i = 0; i < offset; ++i)
	{
		line += src[i] == '\n';
	}
	return line;
}

int32_t token_column(const token_table_t* table, const int32_t index)
{
	int col = 1;
	const int32_t offset = table->offsets[index];
	const char* src = table->src;
	for (int32_t i = 0; i < offset; ++i)
	{
		const int8_t k = src[i] != '\n';
		col = k * (col + 1) + (1 - k);
	}
	return col;
}


struct line_col token_position(const token_table_t* table, const int32_t index)
{
	int32_t line = 1;
	int32_t col = 1;
	const int32_t offset = table->offsets[index];
	const char* src = table->src;
	for (int32_t i = 0; i < offset; ++i)
	{
		line += src[i] == '\n';
		const int8_t k = src[i] != '\n';
		col = k * (col + 1) + (1 - k);
	}
	return (struct line_col){ .line = line, .col = col };
}
