#include "print_diagnostics.h"
#include <stdio.h>
#include "../strings.h"

int32_t tab_depth = 4;

typedef struct src_range_t
{
	int32_t line0;
	int32_t line1;
	int32_t col0;
	int32_t col1;
} src_range_t;


static void get_doc_range(src_range_t* const dest, const char* src, const int32_t offset, const int32_t len)
{
	int32_t line = 1;
	int32_t col = 0;
	for (int32_t i = 0; i <= offset; ++i)
	{
		const char c = src[i];
		line += c == '\n';
		col = c == '\n' ? 0 : c == '\t' ? col + tab_depth : col + 1;
	}
	dest->line0 = line;
	dest->col0 = col;
	for (int32_t i = offset + 1; i < offset + len; ++i)
	{
		const char c = src[i];
		line += c == '\n';
		col = c == '\n' ? 0 : c == '\t' ? col + tab_depth : col + 1;
	}
	dest->line1 = line;
	dest->col1 = col;
}

static void print_src(const int32_t offset, int32_t len, const src_range_t* range, const char* src, const int32_t src_len)
{
	int32_t line = 1;
	int32_t i = 0;
	int32_t offset1 = offset + len;
	for (; i < src_len; ++i)
	{
		const char c = src[i];
		line += c == '\n';
		if (line == range->line0)
		{
			i += c == '\n';
			break;
		}
	}
	printf("\n %4d | ", line);
	for (int32_t j = i; j < src_len; ++j)
	{
		const char c = src[j];
		if (c != '\n') putc(c, stdout);
		else break;
	}
	printf("\n      | ");
	for (int32_t j = i; j < offset; ++j)
	{
		const char c = src[j];
		if (c != '\t') putc(' ', stdout);
		else putc('\t', stdout);
	}
	putc('^', stdout);
	i = offset + 1;
	for (int32_t j = i; j < offset1; ++j)
	{
		const char c = src[j];
		if (c != '\n' && c != '\t')
		{
			putc('~', stdout);
		}
		else if (c == '\t')
		{
			putc('\t', stdout);
		}
		else
		{
			i = j + 1;
			break;
		}
	}
	while (line < range->line1)
	{
		++line;
		printf("\n %4d | ", line);
		for (int32_t j = i; j < src_len; ++j)
		{
			const char c = src[j];
			if (c != '\n') putc(c, stdout);
			else break;
		}
		printf("\n      | ");
		for (int32_t j = i; j < offset1; ++j)
		{
			const char c = src[j];
			if (c != '\n' && c != '\t')
			{
				putc('~', stdout);
			}
			else if (c == '\t')
			{
				putc('\t', stdout);
			}
			else
			{
				i = j + 1;
				break;
			}
		}
	}
	fflush(stdout);
}

static int32_t sprint_src(char* dest, const int32_t offset, int32_t len, const src_range_t* range, const char* src, const int32_t src_len)
{
	int32_t line = 1;
	int32_t i = 0;
	int32_t offset1 = offset + len;
	int32_t size = 0;
	for (; i < src_len; ++i)
	{
		const char c = src[i];
		line += c == '\n';
		if (line == range->line0)
		{
			i += c == '\n';
			break;
		}
	}
	size += sprintf(dest + size, "\n %4d | ", line);
	for (int32_t j = i; j < src_len; ++j)
	{
		const char c = src[j];
		if (c != '\n') dest[size++] = c;
		else break;
	}
	size += sprintf(dest + size, "\n      | ");
	for (int32_t j = i; j < offset; ++j)
	{
		const char c = src[j];
		if (c != '\t') dest[size++] = ' ';
		else dest[size++] = '\t';
	}
	dest[size++] = '^';
	i = offset + 1;
	for (int32_t j = i; j < offset1; ++j)
	{
		const char c = src[j];
		if (c != '\n' && c != '\t')
		{
			dest[size++] = '~';
		}
		// what's gping on here???
		else if (c == '\t')
		{
			dest[size++] = '\t';
		}
		else
		{
			i = j + 1;
			break;
		}
	}
	while (line < range->line1)
	{
		++line;
		size += sprintf(dest + size, "\n %4d | ", line);
		for (int32_t j = i; j < src_len; ++j)
		{
			const char c = src[j];
			if (c != '\n') dest[size++] = c;
			else break;
		}
		size += sprintf(dest + size, "\n      | ");
		for (int32_t j = i; j < offset1; ++j)
		{
			const char c = src[j];
			if (c != '\n' && c != '\t')
			{
				dest[size++] = '~';
			}
			else if (c == '\t')
			{
				dest[size++] = '\t';
			}
			else
			{
				i = j + 1;
				break;
			}
		}
	}
	return size;
}

void diagnostic_note(string* dest, const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len)
{
	src_range_t range;
	get_doc_range(&range, src, offset, len);
	dest->length += sprintf(dest->chars, "%s:%d:%d \e[32mnote:\e[39m %s", path, range.line0, range.col0, msg);
	dest->length += sprint_src(dest->chars + dest->length, offset, len, &range, src, src_len);
	dest->chars[dest->length++] = '\n';
}

void diagnostic_warn(string* dest, const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len)
{
	src_range_t range;
	get_doc_range(&range, src, offset, len);
	dest->length += sprintf(dest->chars, "%s:%d:%d \e[32mwarning:\e[39m %s", path, range.line0, range.col0, msg);
	dest->length += sprint_src(dest->chars + dest->length, offset, len, &range, src, src_len);
	dest->chars[dest->length++] = '\n';
}

void diagnostic_error(string* dest, const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len)
{
	src_range_t range;
	get_doc_range(&range, src, offset, len);
	dest->length += sprintf(dest->chars, "%s:%d:%d \e[32merror:\e[39m %s", path, range.line0, range.col0, msg);
	dest->length += sprint_src(dest->chars + dest->length, offset, len, &range, src, src_len);
	dest->chars[dest->length++] = '\n';
}


void print_note(const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len)
{
	src_range_t range = {};
	get_doc_range(&range, src, offset, len);
	fprintf(stdout, "%s:%d:%d \e[32mnote:\e[39m %s", path, range.line0, range.col0, msg);
	print_src(offset, len, &range, src, src_len);
	printf("\n");
}

void print_warn(const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len)
{
	src_range_t range = {};
	get_doc_range(&range, src, offset, len);
	fprintf(stdout, "%s:%d:%d \e[32mwarning:\e[39m %s", path, range.line0, range.col0, msg);
	print_src(offset, len, &range, src, src_len);
	printf("\n");
}

void print_error(const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len)
{
	src_range_t range = {};
	get_doc_range(&range, src, offset, len);
	printf("%s:%d:%d \e[31merror:\e[39m %s", path, range.line0, range.col0, msg);
	print_src(offset, len, &range, src, src_len);
	printf("\n");
}
