#include "str_builder.h"
#include <stdio.h>
#include <stdlib.h>

#define STR_SIZE 1024

typedef struct str_node {
	struct str_node* next;
	char str[STR_SIZE];
} str_node;

void rec_free(str_node* node)
{
	str_node* temp;
	while (node->next)
	{
		temp = node->next;
		free(node);
		node = temp;
	}
}

char* str_file(FILE* file)
{
	int size = 0;
	str_node* node = malloc(sizeof(str_node));
	str_node* head = node;
	char c;
	while ((c = getc(file)) != EOF)
	{
		node->str[size++ % STR_SIZE] = c;
		if (size % STR_SIZE == 0) {
			str_node* next = malloc(sizeof(str_node));
			node->next = next;
			node = next;
		}
	}
	// NOTE: just leave the last node big who cares? (I do.)
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
