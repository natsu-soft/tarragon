#include "file_sys.h"
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "allocators/arena.h"
#include "error_macros.h"

#ifdef _WIN32
#define DIR_SEPERATOR '\\'
#define ABS_PATH(BUF, PATH, MAX_LEN) _fullpath(BUF, PATH, MAX_LEN)
#else
#define ABS_PATH(BUF, PATH, MAX_LEN) realpath(PATH, BUF) // NOTE: args are swapped
#define DIR_SEPERATOR '/'
#endif

static arena_t fs_arena = { .addr  = NULL};

static arena_t* fs_singleton(void)
{
	if (!fs_arena.addr)
	{
		arena_init(&fs_arena, 1024 * PATH_MAX);
	}
	return &fs_arena;
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

// NOTE: EXCLUDES DIR SEPERATOR
char* fs_dirname(const char* name)
{
	char* res;
	for (int i = strlen(name) - 1; i >= 0; --i)
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
	res = arena_alloc(fs_singleton(), strlen(name) + 1);
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


int fs_size(const char* path)
{
	FILE* file = fopen(path, "r");
	if (!file)
	{
		fprintf(stderr, "Failed to map file \"%s\"!\n", path);
		return NULL;
	}
	else if ((fseek(file, 0, SEEK_END)) == -1) {
		fprintf(stderr, "Failed to seek end of file \"%s\"!\n", path);
		return NULL;
	}
	const int size = ftell(file);
	fseek(file, 0, SEEK_SET);
	fclose(file);
	return size;
}

char* fs_read(const char* path, int* len)
{
	*len = -1;
	FILE* file = fopen(path, "r");
	if (!file)
	{
		fprintf(stderr, "Failed to open file \"%s\"!\n", path);
		return NULL;
	}
	else if ((fseek(file, 0, SEEK_END)) == -1) {
		fprintf(stderr, "Failed to seek end of file \"%s\"!\n", path);
		return NULL;
	}
	*len = ftell(file);
	fseek(file, 0, SEEK_SET);
	char *res = malloc(*len + 1);
	if (!res)
	{
		fprintf(stderr, "Failed to allocate memory for file \"%s\"!\n", path);
		return NULL;
	}
	fread(res, 1, *len, file);
	res[*len] = '\0';
	fflush(file);
	fclose(file);
	return res;
}

// NOTE: DOES NOT CLOSE STREAM
char* fs_read_stream(FILE* fp, int* str_len)
{
	*str_len = -1;
	if ((fseek(fp, 0, SEEK_END)) == -1)
	{
		perror("Failed to seek end of FILE*\n");
		return NULL;
	}
	*str_len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* res = malloc(*str_len + 1);
	fread(res, 1, *str_len, fp);
	res[*str_len] = '\0';
	return res;
}

void fs_write(const char* abs_path, char* data)
{
	FILE* file = fopen(abs_path, "w");
	assertf(file, "Failed to open \"%s\" for writing!", abs_path);
	if (file)
	{
		fprintf(file, "%s", data);
		fflush(file);
		fclose(file);
	}
}

// does not realloc if delim not encountered
// dest must be allocated before calling!
int fs_getline(char* dest, const size_t n, FILE* stream)
{
	for (size_t i = 0; i < n; ++i)
	{
		char c = fgetc(stream);
		/* printf("%c", c); */
		switch (c)
		{
		case '\n':
			dest[i] = '\0';
			return i;
		case EOF:
			dest[i] = '\0';
			fflush(stream);
			return i;
		default:
			dest[i] = c;
		}
	}
	return EOF;
}

int file_sys_test(int argc, char** argv)
{
#ifdef _WIN32
#define REPO_PATH "D:\\Repos\\tarragon"
#define URI "file:///d:/Repos/tarragon/test_stuff/str_file.txt"
#else
#define REPO_PATH "/home/jon/Repos/tarragon"
#define URI "file:////home/jon/Repos/tarragon/test_stuff/str_file.txt"
#endif
	printf("testing file system functions\n");
	int temp = 0;
	char* str_file_abs_path = fs_abs_path( "test_stuff/str_file.txt");
	char* str_file_uri = fs_abs_uri(str_file_abs_path);
	assertf(!strcmp(URI, str_file_uri), "fs_abs_uri failed");
	assertf(!strcmp(fs_read(str_file_abs_path, &temp), "hello!\n"), "str_file test failed"); // have to ! because strcmp returns 0 (false) if equal
	// dirname test
	/* const char* abs_arg0 = fs_abs_path(argv[0]); */
	/* printf("%s\n", abs_arg0); */
	char* cwd = fs_dirname(argv[0]);
	/* printf("cwd: %s\n", cwd); */
	assertf(!strcmp(cwd, REPO_PATH), "fs_dirname failed!");
	// find upwards
	/* file_context_t* context = fs_find_upward("bruh", cwd); */
	/* assertf(context, "find_upwards failed!"); */
	return 0;
}
