#ifndef LANGUAGE_SERVER_H
#define LANGUAGE_SERVER_H

#define CONFIG_NAME "tarragon_config.json"

#include <stdint.h>

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

typedef enum lsp_state_t
{
	LSP_UNINITIALIZED = 0,
	LSP_INITIALIZING,
	LSP_INITIALIZED,
	LSP_SHUTDOWN,
	LSP_EXIT_0,
	LSP_EXIT_1
} lsp_state_t;

typedef struct text_document_t
{
	char* abs_path;
	char* language_id;
	uint32_t version;
} text_document_t;

/* typedef struct lsp_t */
/* { */
/* 	FILE* out; */
/* 	text_document_t* documents; */
/* 	uint32_t num_documents; */
/* 	lsp_state_t state; */
/* } lsp_t; */

/* lsp_t* lsp(void); */

int ls_listen(FILE* in, FILE* out);

char* ls_generate_config(const char* dir);

#endif /* End LANGUAGE_SERVER_H */
