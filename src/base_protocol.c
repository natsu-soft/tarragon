#include "base_protocol.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include "arena.h"
#include "logger.h"
#include "error_macros.h"
#include "scratch.h"


#define KILOBYTE 1024
#define ARENA_OVERHEAD 4 * KILOBYTE
#define MICROSECOND 1000
#define CHAR_NANOSLEEP 100 * MICROSECOND
#define READ_NANOSLEEP 1000 * MICROSECOND

static struct timespec charsleep = { .tv_nsec = CHAR_NANOSLEEP };
static struct timespec readsleep = { .tv_nsec = READ_NANOSLEEP };

static enum {
	CACHING_HEADER,
	WAITING_CONTENT,
	CACHING_CONTENT,
	END_OF_FILE
} decode_state = CACHING_HEADER;

static uint32_t content_length = 0;
static arena_t decode_arena = {0};

__attribute__((constructor)) void bp_init(void)
{
	arena_init(&decode_arena, KILOBYTE);
}

void bp_reset(void)
{
	decode_arena.size = 0;
	content_length = 0;
	decode_state = CACHING_HEADER;
}

void bp_send(FILE* stream, char* content)
{
	#if _WIN32
	fprintf(stream, "Content-Length: %llu\r\n\r\n%s\n", strlen(content), content);
	#else
	fprintf(stream, "Content-Length: %lu\r\n\r\n%s\n", strlen(content), content);
	#endif
	fflush(stream);
}


static int32_t decode_nosleep(FILE* stream, char** out_json)
{
	if (feof(stream))
	{
		decode_state = END_OF_FILE;
		return -1;
	}
	char c;
	char* temp;
	switch (decode_state)
	{
	case CACHING_HEADER:
		c = fgetc(stream);
		switch (c)
		{
		case '\r':
			temp = decode_arena.allocation;
			uint8_t i;
			for (i = 0; i < decode_arena.size && temp[i] != ' '; ++i);
			char* endptr = temp + decode_arena.size;
			content_length = strtoul(temp + i + 1, &endptr, 10);
			if (content_length == 0)
			{
				return -1;
			}
			if (content_length + 1 > decode_arena.cap)
			{
				temp = realloc(&decode_arena.allocation, content_length + ARENA_OVERHEAD);
				assert(temp && "failed to realloc memory for base protocol arena!");
				decode_arena.allocation = temp;
				decode_arena.cap = content_length + ARENA_OVERHEAD;
			}
			decode_arena.size = 0;
			decode_state = WAITING_CONTENT;
			__attribute__((fallthrough));
		default:
			arena_push(&decode_arena, &c, 1);
		}
		break;
	case WAITING_CONTENT:
		decode_arena.size += fread(decode_arena.allocation + decode_arena.size, 1, 3, stream);
		if (decode_arena.size == 4)
		{
			decode_state = CACHING_CONTENT;
			decode_arena.size = 0;
		}
		break;
	case CACHING_CONTENT:
		/* fread()
		Upon  successful completion, fread() shall return the number of elements
		successfully read which is less than nitems only if a read error or end-
		of-file is encountered. If size or nitems is 0, fread() shall  return  0
		and  the  contents  of  the array and the state of the stream remain un-
		changed. Otherwise, if a read error occurs, the error indicator for  the
		stream shall be set, and errno shall be set to indicate the error.
		*/
		decode_arena.size += fread(decode_arena.allocation + decode_arena.size, 1, content_length - decode_arena.size, stream);
		/* arena_push(&decode_arena, &c, 1); */
		if (decode_arena.size == content_length)
		{
			*out_json = decode_arena.allocation;
			return content_length;
		}
		break;
	}
	return 0;
}

int32_t bp_receive_nosleep(FILE* stream, char** out_json)
{
	int32_t res = 0;
	content_length = 0;
	decode_arena.size = 0;
	decode_state = CACHING_HEADER;
	while(!res)
	{
		res = decode_nosleep(stream, out_json);
	}
	return res;
}



void echo(uint32_t n, FILE* in, FILE* out)
{
	LOG("echoing %d", n);
	/* while(c = getc(in), c) */
	for (uint32_t i = 0; i < n; ++i)
	{
		fprintf(out, "%c", getc(in));
	}
}

#define TEST_STRING "{\"testing\":true}"

// segfalting
int base_protocol_test(int argc, char** argv)
{
	printf("testing rpc encoding/decoding\n");
	FILE* fp = fopen("test_stuff/rpc_encode", "w");
	bp_send(fp, TEST_STRING);
	fclose(fp);
	fp = fopen("test_stuff/rpc_encode", "r");
	char* dest;
	int32_t len = bp_receive_nosleep(fp, &dest);
	fclose(fp);
	assertf(len == 16, "bp_consume length mismatch, parsed length: %d\n", len);
	char* content = scratch_alloc(len + 1);
	memcpy(content, dest, len);
	content[len] = '\0';
	printf("length: %d\ncontent: %s\n", len, content);
	assertf(strncmp(TEST_STRING, dest, len) == 0, "bp_content content mismatch");
	// NOTE: IMPOSSIBLE to test--thanks for including \r\n in the standard, Microshit!
	/* printf("initialize request test\n"); */
	/* fp = fopen("test_stuff/initialize_request.json", "r"); */
	/* len = bp_receive_nosleep(fp, &dest); */
	/* fclose(fp); */
	/* assertf(len == 160, "decoded length: %d; expected: 160\n", len); */
	/* content = scratch_calloc(len + 1); */
	/* memcpy(content, dest, len); */
	/* content[len] = '\0'; */
	/* printf("%s\n", content); */
	/* LOG("%s", content); */
	fclose(fp);
	return 0;
}
