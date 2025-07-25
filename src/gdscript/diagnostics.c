#include "diagnostics.h"
/* #include <stdlib.h> */
#include <stdint.h>
#include "print_diagnostics.h"
#include "gdscript_ids.h"
#include "token.h"

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
const int32_t diagnostics_post_flex(token_table_t* const table, const int32_t src_len, const char* path)
{
	int32_t swap = 0;
	const int32_t num_tokens = table->num_tokens;
	const char* src =  table->src;
	int32_t* const offsets = table->offsets;
	int16_t* const lens = table->lens;
	gdscript_id_t* const ids = table->ids;
	const gdscript_id_t fake_delta = GDSCRIPT_SCOPE_START - GDSCRIPT_FAKE_SCOPE_START;
	const gdscript_id_t or_delta = GDSCRIPT_OP_BOOL_OR - GDSCRIPT_OR;
	const gdscript_id_t and_delta = GDSCRIPT_OP_BOOL_AND - GDSCRIPT_AND;
	/* const gdscript_id_t not_delta = GDSCRIPT_OP_BOOL_NOT - GDSCRIPT_NOT; */
	const gdscript_id_t missing_delta = GDSCRIPT_PAREN_CLOSE - GDSCRIPT_MISSING_PAREN_CLOSE;
	const gdscript_id_t nest_delta = GDSCRIPT_COLON - GDSCRIPT_NESTED_COLON;
	for (int32_t i = 0; i < num_tokens; ++i)
	{
		gdscript_id_t id = ids[i];
		if (id < GDSCRIPT_ERROR_THRESHOLD)
		{
			id += (id == GDSCRIPT_FAKE_SCOPE_START || id == GDSCRIPT_FAKE_SCOPE_END) * fake_delta;
			id += (id == GDSCRIPT_OR) * or_delta;
			id += (id == GDSCRIPT_AND) * and_delta;
			/* id += (id == GDSCRIPT_NOT) * not_delta; */ /* NOTE: not is so fucked I have to shunt first before promoting to OP_BOOL_NOT */
			id += (id == GDSCRIPT_NESTED_COLON) * nest_delta;
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
