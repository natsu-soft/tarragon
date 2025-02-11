#ifndef UNZIP_H
#define UNZIP_H

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif


typedef unsigned int uint32_t;

typedef unsigned long size_t;

typedef void* (*unzip_malloc_t)(size_t);

char* unzip(char* buf, size_t buf_len, uint32_t* out_len, unzip_malloc_t malloc_fn);

char* unzip_file(FILE* file, uint32_t* out_len, unzip_malloc_t malloc_fn);

#endif /* End UNZIP_H */
