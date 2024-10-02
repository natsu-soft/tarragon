#ifndef STR_FILE_H
#define STR_FILE_H

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

#define STR_SIZE 1024

typedef struct str_ll {
	struct str_ll* next;
	char str[STR_SIZE];
} str_ll;

char* build_string_and_free(FILE* file);

#endif
