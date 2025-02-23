#ifndef BIN_TREE_H
#define BIN_TREE_H

typedef struct bin_tree_t
{
	struct bin_tree_t* parent;
	struct bin_tree_t* left; // less than
	struct bin_tree_t* right; // greater than
} bin_tree_t;

#endif /* End BIN_TREE_H */
