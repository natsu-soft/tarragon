#ifndef SORTED_LIST_H
#define SORTED_LIST_H

#include <stddef.h>

typedef struct sorted_list_t
{
	struct sorted_list_t* next;
	size_t value;
} sorted_list_t;

void sorted_list_insert(sorted_list_t** list, sorted_list_t* elem);

sorted_list_t* sorted_list_extract(sorted_list_t** list);

void sorted_list_point_relative(sorted_list_t* list);

void sorted_list_point_absolute(sorted_list_t* list);

#endif /* End SORTED_LIST_H */
