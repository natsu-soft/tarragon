#ifndef FILE_SYS_H
#define FILE_SYS_H

typedef struct _iobuf FILE;

typedef struct file_context {
	FILE* file;
	char* dir;
} file_context;

int find_upward(const char* name, char* dir, file_context* res);

char* dirname(const char* name);

char* join_path(const char* dir, const char* filename);

#endif
