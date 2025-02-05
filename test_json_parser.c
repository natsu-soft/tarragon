#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <tree_sitter/api.h>
#include "./src/file_sys.h"

extern unsigned char request_tsquery[];
extern unsigned int request_tsquery_len;
extern unsigned char params_tsquery[];
extern unsigned int params_tsquery_len;

// Declare the `tree_sitter_json` function, which is
// implemented by the `tree-sitter-json` library.
const TSLanguage *tree_sitter_json(void);

const char err_matching[5][32] = {
	"UNMATCHED ERROR TYPE",
	"Syntax",
	"Node Type",
	"Field",
	"Capture",
};

#define QUERY_ASSERT(A) if (A) { fprintf(stderr, "[TSQUERY ERROR] %s:%d  -- %s\n", __FILE__, __LINE__, err_matching[A]); exit(EXIT_FAILURE); }

char* node_text(const TSNode* n, const char* str)
{
	int b0 = ts_node_start_byte(*n);
	int b1 = ts_node_end_byte(*n);
	char* res = malloc(b1 - b0 + 1);
	strncpy(res, str + b0, b1 - b0);
	res[b1 - b0] = '\0';
	return res;
}

TSQuery* make_query(const char* query, const int len)
{
	TSQueryError err;
	uint32_t err_offset;
	TSQuery* ts_q = ts_query_new(
		tree_sitter_json(),
		query,
		len,
		&err_offset,
		&err);
	QUERY_ASSERT(err);
	return ts_q;
}

int main() {
  // Create a parser.
  TSParser *parser = ts_parser_new();

  // Set the parser's language (JSON in this case).
  ts_parser_set_language(parser, tree_sitter_json());

  // Build a syntax tree based on source code stored in a string.
  int len;
  char* src = fs_filestr("initialize.json", &len);
  TSTree *tree = ts_parser_parse_string(
    parser,
    NULL,
    src,
    len
  );
  /* TSQuery* query = make_query(&request_tsquery, request_tsquery_len); */
  TSQuery* query = make_query(&params_tsquery, params_tsquery_len);
  TSQueryCursor* cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));

  TSQueryMatch match;
  TSNode node;
  char* txt;
  int i = 0;
  while(ts_query_cursor_next_match(cursor, &match))
  {
	  if (match.pattern_index == 0)
	  {
		  node = match.captures[0].node;
		  char* txt = node_text(&node, src);
		  /* printf("method: %s\n", txt); */
		  printf("key: %s\n", txt);
		  free(txt);
		  node = match.captures[1].node;
		  txt = node_text(&node, src);
		  /* printf("params: %s\n", txt); */
		  printf("value: %s\n", txt);
		  free(txt);
	  }
	  else if (match.pattern_index == 1)
	  {
		  node = match.captures[0].node;
		  char* txt = node_text(&node, src);
		  /* printf("method: %s\n", txt); */
		  printf("key: %s\n", txt);
		  free(txt);
		  node = match.captures[1].node;
		  txt = node_text(&node, src);
		  /* printf("params: %s\n", txt); */
		  printf("array: %s\n", txt);
		  free(txt);
	  }
	  ++i;
	  /* node = match.captures[0].node; */
	  /* txt = node_text(&node, src); */
	  /* printf("text: %s\n", txt); */
	  /* free(txt); */
  }
  printf( "matches: %d\n", i++);

  // Get the root node of the syntax tree.
  /* TSNode root_node = ts_tree_root_node(tree); */

  // Get some child nodes.
  /* TSNode array_node = ts_node_named_child(root_node, 0); */
  /* TSNode number_node = ts_node_named_child(array_node, 0); */

  // Check that the nodes have the expected types.
  /* assert(strcmp(ts_node_type(root_node), "document") == 0); */
  /* assert(strcmp(ts_node_type(array_node), "array") == 0); */
  /* assert(strcmp(ts_node_type(number_node), "number") == 0); */

  // Check that the nodes have the expected child counts.
  /* assert(ts_node_child_count(root_node) == 1); */
  /* assert(ts_node_child_count(array_node) == 5); */
  /* assert(ts_node_named_child_count(array_node) == 2); */
  /* assert(ts_node_child_count(number_node) == 0); */

  // Print the syntax tree as an S-expression.
  /* char *string = ts_node_string(root_node); */
  /* printf("Syntax tree: %s\n", string); */

  // Free all of the heap-allocated memory.
  ts_query_cursor_delete(cursor);
  ts_query_delete(query);
  free(src);
  /* free(string); */
  ts_tree_delete(tree);
  ts_parser_delete(parser);
  return 0;
}
