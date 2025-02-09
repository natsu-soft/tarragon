#include "gd_trie.h"
#include <stdlib.h>

static trie_malloc_t trie_malloc = &malloc;
static trie_free_t trie_free = &free;


void set_gd_trie_alloc_funcs(trie_malloc_t malloc_fn, trie_free_t free_fn)
{
	trie_malloc = malloc_fn;
	trie_free = free_fn;
}

void add_child(trie_elem_t* elem, trie_elem_t* child)
{
	trie_ll_t* n = trie_malloc(sizeof *n);
	n->elem = child;
	n->next = elem->children;
	elem->children = n;
}

void remove_child(trie_elem_t* elem, trie_elem_t* child)
{
	if (!elem->children)
	{
		return;
	}
	trie_ll_t* node = elem->children;
	do {
		if (node->elem == child)
		{
			// vgh
		}
	} while(node = node->next, node);
}
