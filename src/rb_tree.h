#ifndef RB_TREE_H
#define RB_TREE_H

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
	void* ptr;
	rb_color_t color;
} rb_node_t;

rb_node_t* rbt_insert(rb_node_t* node, void* ptr);

rb_node_t* rbt_remove(rb_node_t* node, void* ptr);

#endif /* End RB_TREE_H */
