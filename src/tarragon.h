#ifndef TARRAGON_H
#define TARRAGON_H

/*
 * NOTE: use nftw() to ignore certain subdirs (them ignored dirs BREHHHH)
 * https://beej.us/guide/bgc/html/#multithreading mutex ml
 * https://beej.us/guide/bgc/html/#setting-up-exit-handlers-with-atexit for cleanup
 */

typedef struct _iobuf FILE;

typedef struct tarragon_t {
	char* project_root;
	/* TODO: add as needed */
} tarragon_t;

int init(FILE* config_fp, tarragon_t* t);

int scan(tarragon_t* t);

#endif
