#ifndef FILE_SYS_H
#define FILE_SYS_H

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

typedef struct filep_context {
	FILE* file;
	char* dir;
} filep_context;

int find_upward(const char* name, char* dir, filep_context* res);

char* dirname(const char* name);

char* join_path(const char* dir, const char* filename);

#endif
