#ifndef LANGUAGE_SERVER_H
#define LANGUAGE_SERVER_H

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

typedef struct Lsp
{
	LspState state;
} Lsp;

int ls_listen(FILE* in,FILE* out);

#endif /* End LANGUAGE_SERVER_H */
