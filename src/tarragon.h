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

typedef struct LSP
{
} LSP;

typedef struct Workspace
{
	char* dir;
} Workspace;

typedef struct Document
{
	char* uri;
} Document;

char* lsp_generate_config(const char* dir);

void lsp_init(FILE* config_fp);

void lsp_scan(Workspace* workspace);

void lsp_encode(char* content, FILE* stream);

#endif
