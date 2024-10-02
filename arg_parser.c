#include <string.h>
#include <stdio.h>
/* #include <dirent.h> */
#include <stdlib.h>
#include <string.h>
#include "src/find_file.h"

#define CONFIG_JSON "tarragon.json"


/*
 * IDEALIZED ARGS
 * --help or -h
 *   prints usage
 * --generate or -g [path/to/extension_api.json]
 *   generates a tarragon.json configuration file
 */

int main(int argc, char** argv) {
	// printf("Hello, world!\n");
	// printf("%d\n", argc);
	// this just prints out all args
	for (int i = 0; i < argc; i++) {
		printf("%d: %s\n", i, argv[i]);
	}
	find_file_t res = { .file=NULL, .dir=NULL };
	char* dir = dirname(argv[0]);
	/* FILE* f = find_upward("foo.txt", dir); */
	if (find_upward(CONFIG_JSON, dir, &res))
	{
		printf("FOUND FILE in %s!", res.dir);
		fclose(res.file);
		free(res.dir);
	}
	else
	{
		fprintf(stderr, "did not find %s :(", CONFIG_JSON);
	}
	free(dir);
	return 0;
}
