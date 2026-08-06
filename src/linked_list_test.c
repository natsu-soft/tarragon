#include "linked_list.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "allocators/scratch.h"
#include "error_macros.h"
#include "macro_bitset.h"


#define NODE_CAP 512
struct test_node
{
	LINKED_TREE(struct test_node);
	int64_t value;
};
IMPL_LINKED_LIST_FUNCS(node, struct test_node);

#define nil (&nil_test_node)
struct test_node nil_test_node =
{
	.next = nil,
	.prev = nil,
	.child = nil
};

#define test struct test_node

static test* tree_root(test* jct)
{
	if (jct->parent == nil)
	{
		return jct;
	}
	else
	{
		for (jct = jct->parent; jct->value != 0; jct = jct->next);
		return tree_root(jct);
	}
}

BITSET(ptr, uint64_t, NODE_CAP >> 6)
static int32_t seen = 0;
static int32_t size = 0;
static struct test_node* test_nodes = NULL;
struct ptr_bitset visited_nodes = {0};
/* static uint32_t visited_nodes[8] = {0}; */

static inline test* alloc(void)
{
	assert(size < NODE_CAP);
	test* res = test_nodes + size++;
	*res = (test)
	{
		.next = res,
		.prev = res,
		.child = nil
	};
	return res;
}

static inline void visit(test* node)
{
	/* visited_nodes[(node - test_nodes) >> 5] |= 1u << ((node - test_nodes) & 0x1f); */
	ptr_bitset_add(&visited_nodes, node - test_nodes);
}

static inline uint64_t visited(test* node)
{
	/* return visited_nodes[(node - test_nodes) >> 5] & 1u << ((node - test_nodes) & 0x1f); */
	return ptr_bitset_has(&visited_nodes, node - test_nodes);
}

static void print_nodes(test* node, const int32_t depth)
{
	fflush(stdout);
	assert(node->value == 0 && "INVALID NODE VISITED");
	assert(!visited(node) && "CYCLE DETECTED");
	visit(node);
	for(test* p = node->next; p != node; p = p->next)
	{
		for (int16_t i = 0; i < depth - 1; ++i)
		{
			printf("| ");
		}
		if (depth > 0)
		{
			if (p->next != node)
			{
				/* printf("|-%d (%#018lx)\n", p->value, p); */
				printf("|-%d (%#018lx)\n", p->value, p);
			}
			else
			{
				printf("+-%d (%#018lx)\n", p->value, p);
			}
		}
		else
		{
			printf("%d (%#018lx)\n", p->value, p);
		}
		if (p->child != nil)
		{
			print_nodes(p->child, depth + 1);
		}
		fflush(stdout);
		assertf(!visited(p), "already visited: %d\n", p->value);
		/* assert(!visited(p) && "CYCLE!"); */
		visit(p);
		++seen;
	}
}

static void visit_nodes(test* node)
{
	assert(node->value == 0 && "INVALID NODE VISITED");
	assert(!visited(node) && "CYCLE DETECTED!");
	visit(node);
	for (test* p = node->next; p != node; p = p->next)
	{
		assert(p->value != 0 && "INVALID NODE VISITED");
		assertf(!visited(p), "CYCLE: %d\n", p->value);
		visit(p);
		++seen;
		if (p->child != nil)
		{
			visit_nodes(p->child);
		}
	}
}


static void node_adopt(struct test_node* parent, struct test_node* child)
{
	if (parent->child == nil)
	{
		parent->child = alloc();
		parent->child->parent = parent;
	}
	node_remove(child);
	child->next = child;
	child->prev = child;
	node_prepend(child, parent->child);
}

static test* real_node(int32_t size0)
{
	test* res;
	do
	{
		res = test_nodes + (rand() % size0);
	} while (res->value == 0);
	return res;
}

static inline test* any_node(int32_t size0)
{
	return test_nodes + (rand() % size0);
}

int linked_list_test(int argc, char** argv)
{
	enum
	{
		PREPEND = 0x00,
		APPEND,
		ADOPT,
		NUM_FUZZY_ACTIONS,
		REPLACE,
	};
	srand(400);
	test_nodes = scratch_alloc(NODE_CAP * sizeof *test_nodes);
	for (int32_t iteration = 0; iteration < 100; ++iteration)
	{
		size = 0;
		test* root = alloc();
		for (int32_t i = 0; i < 256; ++i)
		{
			const int32_t num_nodes = size;
			test* n = alloc();
			n->value = i + 1;
			if (num_nodes > 1)
			{
				test* old_node;
				switch(rand() % NUM_FUZZY_ACTIONS)
				{
				case PREPEND:
					old_node = any_node(num_nodes);
					/* printf("> prepending %d to %d\n", n->value, old_node->value); */
					node_prepend(n, old_node);
					break;
				case APPEND:
					old_node = any_node(num_nodes);
					/* printf("> appending %d to %d\n", n->value, old_node->value); */
					node_append(old_node, n);
					break;
				case ADOPT:
					old_node = real_node(num_nodes);
					/* printf("> adopting %d to %d\n", n->value, old_node->value); */
					node_adopt(old_node, n);
					break;
				case REPLACE:
					old_node = real_node(num_nodes);
					/* printf("> replacing %d w/ %d\n", old_node->value, n->value); */
					node_replace(n, old_node);
					n = alloc();
					node_append(n, old_node);
					break;
				}
			}
			else
			{
				node_append(root, n);
			}
			if (REPLACE > NUM_FUZZY_ACTIONS)
			{
				seen = 0;
				/* memset(visited_nodes, 0x00, NODE_CAP >> 3); */
				memset(visited_nodes.bits, 0x00, NODE_CAP >> 3);
				/* print_nodes(root, 0); */
				visit_nodes(root);
				assert(seen == i + 1 && "DID NOT VISIT ALL NODES!");
			}
			else
			{
				seen = 0;
				memset(visited_nodes.bits, 0x00, NODE_CAP >> 3);
				for (int32_t j = 0; j < size; ++j)
				{
					if ((test_nodes + j)->value != 0)
					{
						continue;
					}
					test* r = tree_root(test_nodes + j);
					if (!visited(r))
					{
						visit_nodes(r);
					}
				}
				/* printf("visited %d/%d\n", seen, i + 1); */
				assert(seen == i + 1 && "DID NOT VISIT ALL NODES!");
			}
		}
	}
	printf(" > ran 100 fuzzy tests w/ %d non-junction nodes\n", NODE_CAP / 2);
	return 0;
}
