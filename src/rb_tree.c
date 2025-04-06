#include "rb_tree.h"


/**********************************************************************************
 * PROPERTIES
 * 1. every node is either RED or BLACK
 * 2. all NULL nodes are cosidered BLACK
 * 3. a RED node does not have a RED child
 * 4. every path from a given node to any of its NULL descendants goes through
 *    the same number of BLACK nodes
 * LEMMA. If a node N has exactly one child, the child must be RED, because
 * 	if it were BLACK, its NULL descendants would sit at a different black
 * 	depth than N's NULL child
 **********************************************************************************/


void rb_insert(rb_node_t** root, rb_node_t* node)
{
	rb_node_t* n = *root;
	rb_node_t** side_ptr = node->value >= n->value ? &n->rhs : &n->lhs;
	if (*side_ptr != NULL)
	{
		rb_insert(side_ptr, node);
		return;
	}
	*side_ptr = node;
	rb_node_t** other_ptr = node->value < n->value ? &n->rhs : &n->lhs;
	if (n->color == RB_RED)
	{
	}
	node->color = RB_RED;

	/* node->color = n->color ^ RB_RED; */
	// rebalance
}

void rb_remove(rb_node_t** root, rb_node_t* node)
{
	/* TODO */
}
