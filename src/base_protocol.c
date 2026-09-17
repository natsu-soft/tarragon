#include "base_protocol.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include "allocators/arena.h"
#include "logger.h"
#include "error_macros.h"
#include "allocators/scratch.h"

// NOTE: this is TRASH--rewrite this


#define ARENA_OVERHEAD 4096
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

/* __attribute__((constructor)) */ void bp_init(void)
{
	arena_init(&decode_arena, ARENA_OVERHEAD);
}

const uint32_t bp_send(FILE* stream, const char* content)
{
	const uint32_t res = strlen(content);
#ifdef _WIN32
	/* fprintf(stream, "Content-Length: %llu\r\n\r\n%s", res, content); */
	fprintf(stream, "Content-Length: %u\n\n%s", res, content);
#else
	fprintf(stream, "Content-Length: %lu\r\n\r\n%s", res, content);
#endif
	fflush(stream);
	return res;
}

const uint32_t bp_sendn(FILE* stream, const char* content, const uint32_t n)
{
#ifdef _WIN32
	/* fprintf(stream, "Content-Length: %llu\r\n\r\n%s", res, content); */
	fprintf(stream, "Content-Length: %u\n\n%s", n, content);
#else
	fprintf(stream, "Content-Length: %lu\r\n\r\n%s", n, content);
#endif
	return n;
}


// TODO: test the windows shit with something other than eglot
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
		// REALLY?  FUCN-A MICROSHIT, YOU'RE NOT FOLLOWING YOUR OWN GUIDELINES.
		// or maybe its just eglot being quirky & charming :)
		/* printf("%c", c == '\r' ? '@' : c == '\n' ? '#' : c); */
		switch (c)
		{
#ifdef _WIN32 // fucc u windows
		case '\n':
#else
		case '\r':
#endif
			temp = decode_arena.allocation;
			uint8_t i;
			for (i = 0; i < decode_arena.size && temp[i] != ' '; ++i);
			char* endptr = temp + decode_arena.size;
			content_length = strtoul(temp + i + 1, &endptr, 10);
			/* LOG("content_length: %d", content_length); */
			if (content_length == 0)
			{
				return -1;
			}
			else if (content_length + 1 > decode_arena.cap)
			{
				temp = realloc(decode_arena.allocation, content_length + ARENA_OVERHEAD);
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
#ifdef _WIN32
		decode_arena.size += fread(decode_arena.allocation + decode_arena.size, 1, 2 - decode_arena.size, stream);
		if (decode_arena.size == 2)
#else
		decode_arena.size += fread(decode_arena.allocation + decode_arena.size, 1, 4 - decode_arena.size, stream);
		if (decode_arena.size == 4)
#endif
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

static uint32_t write_initialize_request_test(void)
{
	FILE* fp = fopen("test_stuff/initialize_request.json", "w");
	uint32_t res = bp_send(fp, "{\"jsonrpc\":\"2.0\",\"id\": 0,\"method\":\"initialize\",\"params\":{\"processId\":null,\"clientInfo\":{\"name\":\"testing\",}\"capabilities\":{}}}");
	fclose(fp);
	return res;
}

#define TEST_STRING "{\"testing\":true}"

int base_protocol_test(int argc, char** argv)
{
	printf(" > testing rpc encoding/decoding\n");
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
	LOG("base_protocol.c: length: %d\ncontent: %s\n", len, content);
	assertf(strncmp(TEST_STRING, dest, len) == 0, "bp_content content mismatch");
	printf(" > initialize request test\n");
	uint32_t req_len = write_initialize_request_test();
	fp = fopen("test_stuff/initialize_request.json", "r");
	len = bp_receive_nosleep(fp, &dest);
	fclose(fp);
	assertf(len == req_len, "decoded length: %d; expected: %d\n", len, req_len);
	content = scratch_alloc(len + 1);
	memcpy(content, dest, len);
	content[len] = '\0';
	printf("%s\n", content);
	LOG("%s", content);
	return 0;
}
