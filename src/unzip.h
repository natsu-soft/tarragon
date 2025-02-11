#ifndef UNZIP_H
#define UNZIP_H

typedef unsigned int uint32_t;

typedef unsigned long size_t;

typedef void* (*unzip_malloc_t)(size_t);

char* unzip(char* buf, int buf_len, uint32_t* out_len, unzip_malloc_t malloc_fn);

#endif /* End UNZIP_H */
