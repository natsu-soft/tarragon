#include "rpc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "arena.h"

static uint32_t content_length;

static enum {
	CACHING_HEADER,
	WAITING_CONTENT,
	CACHING_CONTENT
} decode_state = CACHING_HEADER;

static Arena* decode_arena = NULL;

char* rpc_messsage(void)
{
	return decode_arena->memory;
}

void rpc_recycle(void)
{
	content_length = 0;
	decode_arena->size = 0;
}


void rpc_encode(FILE* stream, char* content)
{
	#if _WIN32
	fprintf(stream, "Content-Length: %llu\r\n\r\n%s", strlen(content), content);
	#else
	fprintf(stream, "Content-Length: %lu\r\n\r\n%s", strlen(content), content);
	#endif
}

uint32_t rpc_decode(FILE* stream)
{
	char c;
	char* temp;
	size_t n;
	if (!decode_arena)
	{
		decode_arena = arena_init(1024);
	}
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
		break;
	case WAITING_CONTENT:
		if (fgetc(stream) == '\n')
		{
			decode_state = CACHING_CONTENT;
		}
		break;
	case CACHING_CONTENT:
		decode_arena->size = fread(decode_arena->memory + decode_arena->size, 1, content_length - decode_arena->size, stream);
		if (decode_arena->size == content_length)
		{
			decode_arena->memory[content_length] = '\0';
			return content_length;
		}
		break;
	}
	return 0;
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
