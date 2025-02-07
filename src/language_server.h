#ifndef LANGUAGE_SERVER_H
#define LANGUAGE_SERVER_H

#include <stdint.h>

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

typedef enum LspState
{
	LSP_UNINITIALIZED = 0,
	LSP_INITIALIZING,
	LSP_INITIALIZED,
	LSP_SHUTDOWN,
	LSP_EXIT_0,
	LSP_EXIT_1
} LspState;

typedef struct TextDocument
{
	char* abs_path;
	char* language_id;
	uint32_t version;
} TextDocument;

typedef struct Lsp
{
	FILE* out;
	TextDocument* documents;
	uint32_t num_documents;
	LspState state;
} Lsp;

Lsp* lsp(void);

int ls_listen(FILE* in,FILE* out);

#endif /* End LANGUAGE_SERVER_H */
