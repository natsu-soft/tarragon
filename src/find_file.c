#include "find_file.h"
#include <stdio.h>
/* #include <dirent.h> */
#include <stdlib.h>
#include <string.h>
/* #include <stdbool.h> */

#ifdef _WIN32
#define DIR_SEPERATOR '\\'
#else
#define DIR_SEPERATOR '/'
#endif

int path_compare(const char*  path0, const char*  path1);

int find_upward(const char* name, char* dir, find_file_t* res)
{
	char* temp = malloc(strlen(dir) + strlen(name) + 2);
	int idx = 0;
	for (size_t i = 0; i < strlen(dir); i++)
	{
		temp[idx++] = dir[i];
	}
	temp[idx++] = DIR_SEPERATOR;
	for (size_t i = 0; i < strlen(name); i++)
	{
		temp[idx++] = name[i];
	}
	temp[idx++] = '\0';
	/* printf("checking: %s...\n", temp); */
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
	if (1 - path_compare(temp, dir))
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

int path_compare(const char* path0, const char* path1)
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
