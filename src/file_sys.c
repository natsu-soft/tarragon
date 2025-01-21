#include "file_sys.h"
#include <stdio.h>
#include <limits.h>
/* #include <dirent.h> */
#include <stdlib.h>
#include <string.h>
/* #include <stdbool.h> */

#ifdef _WIN32
#define DIR_SEPERATOR '\\'
#define ABS_PATH(BUF, PATH, MAX_LEN) _fullpath(BUF, PATH, MAX_LEN)
#else /* NOTE: test this */
#define ABS_PATH(BUF, PATH, MAX_LEN) realpath(PATH, BUF) // NOTE: args are swapped
#define DIR_SEPERATOR '/'
#endif


char *fs_abs_path(const char* rel_path)
{
	char *res = malloc(PATH_MAX * sizeof *res);
	if (ABS_PATH(res, rel_path, PATH_MAX) == NULL)
	{
		fprintf(stderr, "Failed to find absolute path for: %s", rel_path);
		exit(EXIT_FAILURE);

	}
	return res;
}

int path_eq(const char*  path0, const char*  path1);

int find_upward(const char* name, char* dir, filep_context* res)
{
	char* temp = join_path(dir, name);
	FILE* file = fopen(temp, "r");
	if (file !=  NULL)
	{
		res->file = file;
		res->dir = dir;
		/* printf("found %s%c%s!", dir, DIR_SEPERATOR, name); */
		free(temp);
		return 1;
	}
	temp = dirname(dir);
	/* printf("dir: %s\tnext-dir: %s\n", dir, temp); */
	if (1 - path_eq(temp, dir))
	{
		return find_upward(name, temp, res);
	}
	else
	{
		res->file = NULL;
		res->dir = NULL;
		free(temp);
		return 0;
	}
}

// NOTE: test this w/ Unix filesystem
char* dirname(const char* name)
{
	char* res;
	for (int i = strlen(name) - 1; i >= 0; i--)
	{
		switch (name[i])
		{
		case DIR_SEPERATOR:
			res = malloc((i + 1) * sizeof(char));
			memcpy(res, name, i);
			res[i] = '\0';
			return res;
		}
	}
	res = malloc(strlen(name) * sizeof(char));
	memcpy(res, name, strlen(name));
	res[strlen(name)] = '\0';
	return res;
}

char* join_path(const char* dir, const char* name)
{
	char* res = malloc(strlen(dir) + strlen(name) + 2);
	int idx = 0;
	for (size_t i = 0; i < strlen(dir); i++)
	{
		res[idx++] = dir[i];
	}
	res[idx++] = DIR_SEPERATOR;
	for (size_t i = 0; i < strlen(name); i++)
	{
		res[idx++] = name[i];
	}
	res[idx++] = '\0';
	return res;
}

int path_eq(const char* path0, const char* path1)
{
	if (strlen(path0) != strlen(path1))
	{
		return 0;
	}
	for (size_t i = 0; i < strlen(path0); i++)
	{
		if (path0[i] != path1[i])
		{
			return 0;
		}
	}
	return 1;
}

char* str_file(const char* abs_path, int* out_len)
{
	char* res = NULL;
	*out_len = 0;
	long temp;
	FILE* file = fopen(abs_path, "r");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		temp = ftell(file);
		fseek(file, 0, SEEK_SET);
		res = malloc(*out_len);
		if (res)
		{
			fread(res, 1, temp, file);
			*out_len = temp;
		}
	}
	return res;
}
