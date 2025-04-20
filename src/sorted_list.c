#include "sorted_list.h"
#include "pointer_memes.h"
#include <assert.h>
#include <stdio.h>
/* #include <stdlib.h> */

// big-O nerds SCREAMING rn
void sorted_list_insert(sorted_list_t** list, sorted_list_t* elem)
{
	const size_t elem_val = elem->value;
	for (; *list != NULL; list = &((*list)->next))
	{
		if (elem_val >= (*list)->value)
		{
			elem->next = *list;
			break;
		}
	}
	*list = elem;
}

sorted_list_t* sorted_list_extract(sorted_list_t** list)
{
	sorted_list_t* res = *list;
	*list = res->next;
	res->next = NULL;
	return res;
}

// UNFORTUNATELY, this HAS to recurse (unless you want to use a stack)
void sorted_list_point_relative(sorted_list_t* list)
{
	sorted_list_t* temp;
	for ( ; list != NULL; list = temp)
	{
		temp = list->next;
		/* printf("(REL %#zx: %#zx", list, list->next); */
		if (temp)
		{
			point_relative(&list->next);
			/* printf(" -> %#zx", list->next); */
		}
		/* printf(")\n"); */
	}
}

void sorted_list_point_absolute(sorted_list_t* list)
{
	for (; list != NULL; list = list->next)
	{
		/* printf("(ABS %#zx: %#zx", list, list->next); */
		if (list->next != NULL)
		{
			point_absolute(&list->next);
			/* printf(" -> %#zx", list->next); */
		}
		/* printf(")\n"); */
	}
}
