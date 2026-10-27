#ifndef SYNTAX_CHECK_H
#define SYNTAX_CHECK_H

#include "source_data_types.h"
#include "../bitset_types.h"

void syntax_check(struct bitset* hoist_set, struct bitset* semantic_set, source_data_t* src_data);



#endif /* End SYNTAX_CHECK_H */
