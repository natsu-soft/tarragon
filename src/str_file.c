#include "str_file.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct str_ll {
	struct str_ll* next;
	char str[STR_SIZE];
} str_ll;

void rec_free(str_ll* node)
{
	if (node->next != NULL)
	{
		rec_free(node->next);
	}
	free(node);
}

char* build_string_and_free(FILE* file)
{
	unsigned int size = 0;
	str_ll* node = malloc(sizeof(str_ll));
	str_ll* head = node;
	char c;
	while ((c = fgetc(file)) != EOF)
	{
		node->str[size++ % STR_SIZE] = c;
		if (size % STR_SIZE == 0) {
			str_ll* next = malloc(sizeof(str_ll*));
			node->next = next;
			node = next;
		}
	}
	// NOTE: just leave the last node kinda big who cares? (I do.)
	// realloc_last_str((char**)&(node->str), (size % STR_SIZE));
	node->next = NULL;
	char* res = malloc((size + 1) * sizeof *res);
	node = head;
	unsigned int idx = 0;
	while (node->next != NULL)
	{
		for (int i = 0; i < STR_SIZE; i++)
		{
			res[idx * STR_SIZE + i] = node->str[i];
		}
		idx++;
		node = node->next;
	}
	for (int i = 0; i < size % STR_SIZE; i++)
	{
		res[idx * STR_SIZE + i] = node->str[i];
	}
	res[size] = '\0';
	rec_free(head);
	fclose(file);
	return res;
}
