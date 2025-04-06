#ifndef RB_TREE_H
#define RB_TREE_H


// https://adtinfo.org/libavl.html/Red_002dBlack-Trees.html#Red_002dBlack-Trees

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

#include <stddef.h>

typedef enum rb_color_t
{
	RB_BLACK = 0x00,
	RB_RED = 0x01
} rb_color_t;

typedef struct rb_node_t
{
	struct rb_node_t* parent;
	struct rb_node_t* lhs;
	struct rb_node_t* rhs;
	int value;
	rb_color_t color;
} rb_node_t;

void rb_insert(rb_node_t** root, rb_node_t* node);

void rb_remove(rb_node_t** root, rb_node_t* node);

#endif /* End RB_TREE_H */
