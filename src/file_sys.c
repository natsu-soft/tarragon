#include "file_sys.h"
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "arena.h"
#include "error_macros.h"
/* #include <stdbool.h> */

#ifdef _WIN32
#define DIR_SEPERATOR '\\'
#define ABS_PATH(BUF, PATH, MAX_LEN) _fullpath(BUF, PATH, MAX_LEN)
#else
#define ABS_PATH(BUF, PATH, MAX_LEN) realpath(PATH, BUF) // NOTE: args are swapped
#define DIR_SEPERATOR '/'
#endif

static arena_t* fs_arena = NULL;

static arena_t* fs_singleton(void)
{
	if (!fs_arena)
	{
		fs_arena = arena_init(1024 * PATH_MAX);
	}
	return fs_arena;
}

char* fs_abs_uri(const char* abs_path)
{
	char* res = arena_alloc(fs_singleton(), strlen(abs_path) + 9);
	if (sprintf(res, "file:///%s", abs_path) > 0)
	{
		#ifdef _WIN32
		for (int i = 8; i < strlen(res); ++i)
		{
			switch(res[i])
			{
			case DIR_SEPERATOR:
				res[i] = '/';
				break;
			}
		}
		#endif
		return res;
	}
	else
	{
		return NULL;
	}
}


char *fs_abs_path(const char* rel_path)
{
	char *res = arena_alloc(fs_singleton(), PATH_MAX);
	if (ABS_PATH(res, rel_path, PATH_MAX) == NULL)
	{
		fprintf(stderr, "Failed to find absolute path for: %s", rel_path);
		exit(EXIT_FAILURE);

	}
	arena_free(fs_singleton(), PATH_MAX - (strlen(res) + 1));
	return res;
}


// oh yeah im just writing my own dir shit here BECAUSE THIS IS WHERE I USE IT GODDAMNIT
file_context_t* fs_find_upward(const char* name, char* dir)
{
	char* temp = fs_join(dir, name);
	size_t size0 = strlen(temp);
	FILE* fp = fopen(temp, "r");
	if (fp)
	{
		arena_free(fs_singleton(), strlen(temp) + 1);
		file_context_t* res = arena_alloc(fs_singleton(), sizeof *res);
		res->fp = fp;
		res->dir = dir;
		return res;
	}
	for (int i = strlen(dir) - 1; i >= 0; i--)
	{
		switch(dir[i])
		{
		case DIR_SEPERATOR:
			memmove(&temp[i + 1], name, strlen(name) + 1);
			arena_free(fs_singleton(), size0 - strlen(temp));
			fp = fopen(temp, "r");
			if (fp)
			{
				arena_free(fs_singleton(), strlen(temp) + 1);
				temp = arena_alloc(fs_singleton(), i + 1);
				memmove(temp, dir, i);
				temp[i] = '\0';
				file_context_t* res = arena_alloc(fs_singleton(), sizeof *res);
				res->fp = fp;
				res->dir = temp;
				return res;
			}
			break;
		}
	}
	return NULL;
}

// NOTE: test this w/ Unix filesystem
char* fs_dirname(const char* name)
{
	char* res;
	for (int i = strlen(name) - 1; i >= 0; i--)
	{
		switch (name[i])
		{
		case DIR_SEPERATOR:
			res = arena_alloc(fs_singleton(), i + 1);
			memmove(res, name, i);
			res[i] = '\0';
			return res;
		}
	}
	res = arena_alloc(fs_singleton(), strlen(name));
	memmove(res, name, strlen(name));
	res[strlen(name)] = '\0';
	return res;
}

char* fs_join(const char* dir, const char* name)
{
	/* char* res = malloc(strlen(dir) + strlen(name) + 2); */
	char* res = arena_alloc(fs_singleton(), strlen(dir) + strlen(name) + 2);
	memmove(res, dir, strlen(dir));
	res[strlen(dir)] = DIR_SEPERATOR;
	memmove(&res[strlen(dir) + 1], name, strlen(name));
	res[strlen(dir) + strlen(name) + 1] = '\0';
	return res;
}

// NOTE: HELLO, MEMORY MANAGER? WHERE'S THE ARENA???
char* fs_filestr(const char* abs_path, int* out_len)
{
	char* res = NULL;
	*out_len = 0;
	FILE* file = fopen(abs_path, "r");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		*out_len = ftell(file);
		fseek(file, 0, SEEK_SET);
		res = malloc(*out_len + 1);
		if (res)
		{
			fread(res, 1, *out_len, file);
			res[*out_len] = '\0';
		}
	}
	fclose(file);
	return res;
}

void fs_write(const char* abs_path, char* data)
{
	FILE* file = fopen(abs_path, "w");
	assertf(file, "Failed to open \"%s\" for writing!", abs_path);
	if (file)
	{
		fprintf(file, "%s", data);
		fclose(file);
	}
}
