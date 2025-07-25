#include "diagnostics.h"
/* #include <stdlib.h> */
#include <stdint.h>
#include "print_diagnostics.h"
#include "gdscript_ids.h"
#include "../macro_stack.h"
STACK(id, gdscript_id_t)
#include "token.h"
#include "../allocators/scratch.h"

diagnostics_output_t diagnostics_out = 0x00;


void diagnostic_note(const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len)
{
	if (diagnostics_out & DIANGOSTICS_JSONRPC)
	{
	}
	else if (diagnostics_out & DIAGNOSTICS_STDOUT)
	{
		print_note(msg, src, src_len, path, offset, len);
	}
	/* TODO */
}

void diagnostic_warn(const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len)
{
	if (diagnostics_out & DIANGOSTICS_JSONRPC)
	{
	}
	else if (diagnostics_out & DIAGNOSTICS_STDOUT)
	{
		print_warn(msg, src, src_len, path, offset, len);
	}
}

void diagnostic_error(const char* msg, const char* src, const int32_t src_len, const char* path, const int32_t offset, const int16_t len)
{
	if (diagnostics_out & DIANGOSTICS_JSONRPC)
	{
	}
	else if (diagnostics_out & DIAGNOSTICS_STDOUT)
	{
		print_error(msg, src, src_len, path, offset, len);
	}
}

// turn missing things into regular ones
// KILL all the other ones
const int32_t diagnostics_pre_analysis(token_table_t* const table, const int32_t src_len, const char* path)
{
	const int32_t num_tokens = table->num_tokens;
	const char* src =  table->src;
	int32_t* offsets = table->offsets;
	int16_t* lens = table->lens;
	gdscript_id_t* ids = table->ids;
	/* const gdscript_id_t not_delta = GDSCRIPT_OP_BOOL_NOT - GDSCRIPT_NOT; */
	const gdscript_id_t missing_delta = GDSCRIPT_PAREN_CLOSE - GDSCRIPT_MISSING_PAREN_CLOSE;
	int32_t swap = 0;
	for (int32_t i = 0; i < num_tokens; ++i)
	{
		gdscript_id_t id = ids[i];
		if (id < GDSCRIPT_ERROR_THRESHOLD)
		{
			offsets[swap] = offsets[i];
			lens[swap] = lens[i];
			ids[swap] = id;
			++swap;
		}
		else if (id == GDSCRIPT_MISSING_PAREN_CLOSE || id == GDSCRIPT_MISSING_BRACE_CLOSE || id == GDSCRIPT_MISSING_BRACKET_CLOSE)
		{
			diagnostic_error("Missing a closing pair. Inserted pair for syntax analysis.", src, src_len, path, offsets[i], lens[i]);
			id += missing_delta; // this works better for some god forsaken reason
			offsets[swap] = offsets[i];
			lens[swap] = lens[i];
			ids[swap] = id;
			++swap;
		}
		else
		{
			switch(id)
			{
#define X(ID) case GDSCRIPT_##ID: diagnostic_error("Found "#ID " during parsing.  Removed before syntax analysis.", src, src_len, path, offsets[i], lens[i]); break;
				GDSCRIPT_ERRORS
#undef X
			}
		}
	}
	table->num_tokens = swap;
	return swap;
}
