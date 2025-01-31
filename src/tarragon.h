#ifndef TARRAGON_H
#define TARRAGON_H

#define CONFIG_NAME "tarragon_config.json"

/*
 * NOTE: use nftw() to ignore certain subdirs (them ignored dirs BREHHHH)
 * https://beej.us/guide/bgc/html/#multithreading mutex ml
 * https://beej.us/guide/bgc/html/#setting-up-exit-handlers-with-atexit for cleanup
 */

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

typedef struct tarragon_t {
	char* project_root;
	/* TODO: add as needed */
} tarragon_t;

char* generate_config(const char* dir);

void init(FILE* config_fp, tarragon_t* t);

void scan(tarragon_t* t);

#endif
