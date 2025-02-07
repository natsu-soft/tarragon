#ifndef FILE_SYS_H
#define FILE_SYS_H

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

typedef struct file_context_t
{
	char* dir;
	FILE* fp;
} file_context_t;

char* fs_abs_uri(const char* abs_path);

char* fs_abs_path(const char* rel_path);

file_context_t* fs_find_upward(const char* name, char* dir);

char* fs_dirname(const char* name);

char* fs_join(const char* dir, const char* filename);

char* fs_filestr(const char* abs_path, int* out_len);

void fs_write(const char* abs_path, char* data);

#endif
