#include "diagnostics.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "../atom_ring.h"
#include "../allocators/arena.h"
#include "../allocators/scratch.h"
#include "../strings.h"
#include "../memery.h"

struct diagnostic
{
	const char* src;
	const char* path;
	int32_t src_len;
	int32_t offset;
	int16_t len;
	enum diagnostic_msg msg;
};

struct src_range
{
	int32_t line0;
	int32_t line1;
	int32_t col0;
	int32_t col1;
};

static void atom_ring_flush_diagnostics(void* addr0, int32_t byte_len);
static void get_src_range(struct src_range* dest, const char* src, const int32_t src_len, const int32_t offset, const int32_t len);
static void print_src(arena_t* dest, char* src, const int32_t src_len, const struct src_range* range);
static void print_error(arena_t* dest, char* message, char* const src, const int32_t src_len, const char* path, const struct src_range* range);
static void print_warn(arena_t* dest, char* message, char* const src, const int32_t src_len, const char* path, const struct src_range* range);
static void print_note(arena_t* dest, char* message, char* const src, const int32_t src_len, const char* path, const struct src_range* range);

#define RING_SIZE 4096
static struct atom_ring diagnostics_ring;

void diagnostics_init(void)
{
	ring_map((struct ring_buffer*)&diagnostics_ring, RING_SIZE, "diagnostics_ring");
	assert(diagnostics_ring.size == (1 << __builtin_ctz(diagnostics_ring.size)));
	assert(diagnostics_ring.size == RING_SIZE);
}

void diagnostics_add(const enum diagnostic_msg msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len)
{
	assert(msg != DIAGNOSTIC_NONE);
	struct diagnostic diagnostic =
	{
		.msg = msg,
		.src = src,
		.path = path,
		.src_len = src_len,
		.offset = offset,
		.len = len
	};
	atom_ring_flushing_concurrent_write(&diagnostics_ring, &diagnostic, sizeof diagnostic, atom_ring_flush_diagnostics);
}

void flush_diagnostics(void)
{
	atom_ring_flush(&diagnostics_ring, atom_ring_flush_diagnostics);
}

static void atom_ring_flush_diagnostics(void* addr0, int32_t byte_len)
{
	arena_t* scratch = scratch_arena(4096);
	for (int32_t i = 0; i < byte_len; i += sizeof(struct diagnostic))
	{
		struct diagnostic* d = (struct diagnostic*)(addr0 + i);
		struct src_range range;
		get_src_range(&range, d->src, d->src_len, d->offset, d->len);
		switch(d->msg)
		{
		case DIAGNOSTIC_NONE:
			assert(0 && "innapopriate value DIAGNOSTIC_NONE in atom_ring_flush_diagnostics()");
			break;
#define X(ID, PRINT_STR) case DIAGNOSTIC_##ID: print_error(scratch, PRINT_STR, d->src, d->src_len, d->path, &range); break;
			ERROR_DIAGNOSTICS
#undef X
#define X(ID, PRINT_STR) case DIAGNOSTIC_##ID: print_warn(scratch, PRINT_STR, d->src, d->src_len, d->path, &range); break;
			WARNING_DIAGNOSTICS
#undef X
#define X(ID, PRINT_STR) case DIAGNOSTIC_##ID: print_note(scratch, PRINT_STR, d->src, d->src_len, d->path, &range); break;
			NOTE_DIAGNOSTICS
#undef X
		}
	}
	printf("%.*s", scratch->size, scratch->addr);
}

int32_t tab_depth = 4;

static void get_src_range(struct src_range* dest, const char* src, const int32_t src_len, const int32_t offset, const int32_t len)
{
	int32_t line = 1;
	int32_t col = 0;
	for (int32_t i = 0; i <= offset && i < src_len; ++i)
	{
		const char c = src[i];
		line += c == '\n';
		col = c == '\n' ? 0 : c == '\t' ? col + tab_depth : col + 1;
	}
	dest->line0 = line;
	dest->col0 = col;
	for (int32_t i = offset + 1; i < offset + len && i < src_len; ++i)
	{
		const char c = src[i];
		line += c == '\n';
		col = c == '\n' ? 0 : c == '\t' ? col + tab_depth : col + 1;
	}
	dest->line1 = line;
	dest->col1 = col;
}

static char* test_src = "lol lmao even";

static void print_src(arena_t* dest, char* const src, const int32_t src_len, const struct src_range* range)
{
	int32_t line = 1;
	int32_t i = 0;
	for (; i < src_len && line < range->line0; ++i)
	{
		line += src[i] == '\n';
	}
	for (++i; line < range->line1 && i <= src_len; ++line, ++i)
	{
		arena_build_string_vsprintf(dest, "%5d | ", line);
		const int32_t i0 = i;
		for (; i < src_len && i != '\n'; ++i) { ; }
		arena_build_string_puts(dest, &STRINGN(src + i0, i - i0));
		arena_build_string_puts(dest, &STRINGN("\n      | ", 9));
		const int32_t underline0 = line == range->line0 ? range->col0 : 0;
		const int32_t underline1 = line == range->line1 ? range->col1 : i - i0;
		// leading whitespace
		for (int32_t c = 0; c < underline0 && i0 + c < src_len; ++c)
		{
			arena_build_string_putc(dest, src[i0 + c] == '\t' ? '\t' : ' ');
		}
		arena_build_string_putc(dest, '^');
		for (int32_t c = underline0 + 1; c < underline1 && i0 + c < src_len; ++c)
		{
			if (src[i0 + c] != '\t')
			{
				arena_build_string_putc(dest, '~');
			}
			else
			{
				for (int32_t j = 0; j < tab_depth; ++j)
				{
					arena_build_string_putc(dest, '~');
				}
			}
		}
		arena_build_string_putc(dest, '\n');
	}
}

static void print_error(arena_t* dest, char* message, char* const src, const int32_t src_len, const char* path, const struct src_range* range)
{
	if (src != test_src)
	{
		arena_build_string_vsprintf(dest, "%s:%d:%d \e[31merror:\e[39m %s\n", path, range->line0, range->col0, message);
		print_src(dest, src, src_len, range);
	}
	else
	{
		arena_build_string_vsprintf(dest, "%s \e[31mtest error:\e[39m %s\n", path, message);
	}
}

static void print_warn(arena_t* dest, char* message, char* const src, const int32_t src_len, const char* path, const struct src_range* range)
{
	if (src != test_src)
	{
		arena_build_string_vsprintf(dest, "%s:%d:%d \e[33mwarning:\e[39m %s\n", path, range->line0, range->col0, message);
		print_src(dest, src, src_len, range);
	}
	else
	{
		arena_build_string_vsprintf(dest, "%s \e[33mtest warning:\e[39m %s\n", path, message);
	}
}

static void print_note(arena_t* dest, char* message, char* const src, const int32_t src_len, const char* path, const struct src_range* range)
{
	if (src != test_src)
	{
		arena_build_string_vsprintf(dest, "%s:%d:%d \e[32mnote:\e[39m %s\n", path, range->line0, range->col0, message);
		print_src(dest, src, src_len, range);
	}
	else
	{
		arena_build_string_vsprintf(dest, "%s\e[32mtest note:\e[39m %s\n", path, message);
	}
}


int diagnostics_test(char** argv, int argc)
{
	for (int32_t i = DIAGNOSTIC_NONE + 1; i < DIAGNOSTIC_SIZE; ++i)
	{
		diagnostics_add(i, test_src, 0, "src/gdscrit/diagnostics.c::diagnostics_test()", 0, 0);
	}
	flush_diagnostics();
	return 0;
}
