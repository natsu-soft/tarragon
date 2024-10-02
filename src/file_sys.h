#ifndef FILE_SYS_H
#define FILE_SYS_H

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

typedef struct file_context {
	FILE* file;
	char* dir;
} file_context;

int find_upward(const char* name, char* dir, file_context* res);

char* dirname(const char* name);

char* join_path(const char* dir, const char* filename);

#endif
