#ifndef STR_BUILDER_H
#define STR_BUILDER_H

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

char* str_file(FILE* file);

#endif
