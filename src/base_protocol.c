#include "base_protocol.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "arena.h"
#include "logger.h"


#define KILOBYTE 1024
#define MICROSECOND 1000
#define CHAR_NANOSLEEP 100 * MICROSECOND
#define READ_NANOSLEEP 1000 * MICROSECOND

static uint32_t content_length;
static struct timespec charsleep = { .tv_nsec = CHAR_NANOSLEEP };
static struct timespec readsleep = { .tv_nsec = READ_NANOSLEEP };

static enum {
	CACHING_HEADER,
	WAITING_CONTENT,
	CACHING_CONTENT,
	END_OF_FILE
} decode_state = CACHING_HEADER;


static Arena* decode_arena = NULL;
char* bp_content(void)
{
	return decode_arena->memory;
}

void bp_init(void)
{
	decode_arena = arena_init(4 * KILOBYTE);
}

void bp_reset(void)
{
	decode_state = CACHING_HEADER;
	content_length = 0;
	decode_arena->size = 0;
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

static int32_t decode(FILE* stream)
{
	if (feof(stream))
	{
		decode_state = END_OF_FILE;
		return -1;
	}
	char c;
	char* temp;
	size_t n;
	switch (decode_state)
	{
	case CACHING_HEADER:
		c = fgetc(stream);
		fprintf(log_fp, "%c", c);
		switch (c)
		{
		case '\n':
			temp = arena_alloc(decode_arena, 1);
			*temp = '\0';
			sscanf(decode_arena->memory, "Content-Length: %u", &content_length);
			decode_arena->size = 0;
			if (content_length + 1 > decode_arena->cap)
			{
				free(decode_arena->memory); // dont use realloc because I dont need to copy
				decode_arena->size = 0;
				decode_arena->cap = content_length + 1;
				decode_arena->memory = malloc(content_length + 1);
			}
			decode_state = WAITING_CONTENT;
			break;
		default:
			temp = arena_alloc(decode_arena, 1);
			*temp = c;
		}
		nanosleep(&charsleep, NULL);
		break;
	case WAITING_CONTENT:
		if (fgetc(stream) == '\n')
		{
			decode_state = CACHING_CONTENT;
		}
		nanosleep(&charsleep, NULL);
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
		decode_arena->size = fread(decode_arena->memory + decode_arena->size, 1, content_length - decode_arena->size, stream);
		if (decode_arena->size == content_length)
		{
			decode_arena->memory[content_length] = '\0';
			fprintf(log_fp, "%s", decode_arena->memory);
			decode_arena->size++;
			return content_length;
		}
		nanosleep(&readsleep, NULL);
		break;
	}
	return 0;
}

static int32_t decode_nosleep(FILE* stream)
{
	if (feof(stream))
	{
		decode_state = END_OF_FILE;
		return -1;
	}
	char c;
	char* temp;
	size_t n;
	switch (decode_state)
	{
	case CACHING_HEADER:
		c = fgetc(stream);
		switch (c)
		{
		case '\n':
			temp = arena_alloc(decode_arena, 1);
			*temp = '\0';
			sscanf(decode_arena->memory, "Content-Length: %u", &content_length);
			decode_arena->size = 0;
			if (content_length + 1 > decode_arena->cap)
			{
				free(decode_arena->memory); // dont use realloc because I dont need to copy
				decode_arena->size = 0;
				decode_arena->cap = content_length + 1;
				decode_arena->memory = malloc(content_length + 1);
			}
			decode_state = WAITING_CONTENT;
			break;
		default:
			temp = arena_alloc(decode_arena, 1);
			*temp = c;
		}
		/* nanosleep(&charsleep, NULL); */
		break;
	case WAITING_CONTENT:
		if (fgetc(stream) == '\n')
		{
			decode_state = CACHING_CONTENT;
		}
		/* nanosleep(&charsleep, NULL); */
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
		decode_arena->size = fread(decode_arena->memory + decode_arena->size, 1, content_length - decode_arena->size, stream);
		if (decode_arena->size == content_length)
		{
			decode_arena->memory[content_length] = '\0';
			decode_arena->size++;
			return content_length;
		}
		/* nanosleep(&readsleep, NULL); */
		break;
	}
	return 0;
}


int32_t bp_receive(FILE* stream)
{
	int32_t n = 0;
	/* LOG("starting bp_receive()"); */
	while(!n)
	{
		n = decode(stream);
	}
	/* LOG("bp_receive: %s", decode_arena->memory); */
	return n;
}

int32_t bp_receive_nosleep(FILE* stream)
{
	int32_t res = 0;
	while(!res)
	{
		res = decode_nosleep(stream);
	}
	/* LOG("bp_receive_nosleep: %s", decode_arena->memory); */
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



/* void rpc_decode2(FILE* stream, uint32_t* content_len, char** content) */
/* { */
/* 	char* header = NULL; */
/* 	size_t n; */
/* 	/\* if (getdelim(&header, &n, '\r', stream) > 0) *\/ */
/* 	if (getline(&header, &n, stream) > 0) */
/* 	{ */
/* 	} */
/* 	if (fscanf(stream, "Content-Length: %u\r\n\r\n", content_len) != 1) */
/* 	{ */
/* 		*content_len = 0; */
/* 		*content = NULL; */
/* 	} */
/* 	char* temp = malloc(*content_len + 1); // OwO Arena??? */
/* 	/\* size_t n; *\/ */
/* 	struct timespec t = { .tv_nsec = 100 }; */
/* 	while(n = fread(temp, 1, *content_len, stream), n < *content_len) // RARE! */
/* 	{ */
/* 		for (int i = n; i >= 0; --i) */
/* 		{ */
/* 			ungetc(temp[i] , stream); */
/* 		} */
/* 		nanosleep(&t, NULL); // ERROR HANDLING MY ASS */
/* 	} */
/* 	temp[*content_len] = '\0'; */
/* 	*content = temp; */
/* } */
