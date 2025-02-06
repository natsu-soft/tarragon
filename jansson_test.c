#include <stdio.h>
#include <stdlib.h>
#include <jansson.h>
#include "src/file_sys.h"

int main(int argc, char** argv)
{
	int len;
	char* text = fs_filestr("initialize.json", &len);
	json_t* root;
	json_error_t error;
	root = json_loads(text, 0, &error);
	/* printf("%s\n", text); */
	free(text);
	if(!root)
	{
		fprintf(stderr, "error on line %d: %s\n", error.line, error.text);
		return 1;
	}
	else if (!json_is_object(root))
	{
		fprintf(stderr, "error: root is not an object!\n");
		json_decref(root);
		return 1;
	}
	void* iter = json_object_iter(root);
	while(iter)
	{
		printf("key: %s\n", json_object_iter_key(iter));
		printf("value: ");
		json_dumpf(json_object_iter_value(iter), stdout, JSON_ENCODE_ANY);
		printf("\n");
		iter = json_object_iter_next(root, iter);
	}
	json_decref(root);
	return 0;
}
