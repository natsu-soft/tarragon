#ifndef TOKEN_TYPES_H
#define TOKEN_TYPES_H
#include <stdint.h>

enum slim_magic
{
	SM_THRESHOLD				= -1,
	SM_EOF						= -5,
	SM_CLOSE_SCOPE				= -10,
	SM_OPEN_SCOPE,
	SM_BAD_LWS					= -20,
	SM_UNCLOSED_PAREN			= -30,
	SM_UNCLOSED_BRACE,
	SM_UNCLOSED_BRACKET,
	SM_EXTRA_PAREN,
	SM_EXTRA_BRACE,
	SM_EXTRA_BRACKET,
	SM_CHARS_AFTER_CONTINUATION	= -40,
	SM_UNTERMINATED_STRING		= -50
};

typedef struct slim_table_t
{
	char* src;
	int32_t length;
	int32_t* offsets;
	int32_t num_tokens;
} slim_table_t;

typedef struct token_table_t {
	char* src;
	int32_t* offsets;
	int16_t* lens;
	enum token_id_t* ids;
	int32_t num_tokens;
} token_table_t;

struct line_col
{
	int32_t line;
	int32_t col;
};


#endif /* End TOKEN_TYPES_H */
