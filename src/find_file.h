#ifndef FIND_FILE_H
#define FIND_FILE_H

typedef struct _iobuf FILE;

typedef struct find_file_t {
	FILE* file;
	char* dir;
} find_file_t;

int find_upward(const char* name, char* dir, find_file_t* res);

char* dirname(const char* name);

#endif
