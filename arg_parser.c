#include <string.h>
#include <stdio.h>
/* #include <dirent.h> */
#include <stdlib.h>


/*
 * IDEALIZED ARGS
 * --help or -h
 * --extension_api or -x path to extension api json file
 *   if not provided searches pwd
 * --cpp or -c path to dir containing ALL GDExtension code
 *   if not provided searches recursively from pwd
 * [path to project.godot] (optional).  If omitted, search up for it.
 */

int main(int argc, char** argv) {
	printf("Hello, world!\n");
	printf("%d\n", argc);
	// this just prints out all args
	for (int i = 0; i < argc; i++) {
		printf("%d: %s\n", i, argv[i]);
	}
	// NOW, we're COOKING
	enum { NOTHING = 0, EXTENSION_API = 1 << 0, CPP_DIR = 1 << 1 } arguments = NOTHING;
	char* api_json = NULL;
	char* cpp_dir = NULL;
	int idx = 1;
	for (; idx < argc && argv[idx][0] == '-';  idx++) {
		switch (argv[idx][1]) {
		case 'h':
			goto usage;
		case 'x':
			arguments |= EXTENSION_API;
			/* api_json = fopen(argv[idx + 1], "r"); */
			api_json = argv[idx + 1];
			if (api_json == NULL) {
				goto json_dne;
			}
			break;
		case 'c':
			arguments |= CPP_DIR;
			cpp_dir = argv[idx + 1];
			if (idx + 1 == argc) {
				goto cpp_dne;
			}
			/* if (cpp_dir == NULL) { */
			/* 	goto cpp_dne; */
			/* } */
			break;
		default:
			goto unrecognized;
		}
	}
	if (arguments & EXTENSION_API & CPP_DIR == EXTENSION_API & CPP_DIR) {
		// all set continue on
	}
	argv += idx;
	free(api_json);
	free(cpp_dir);
	return 0;
usage:
	printf(" bla bla bla");
	return 0;
unrecognized:
	printf("UNRECOGNIZED ARGUMENT: %s", argv[idx]);
	goto cleanup;
json_dne:
	printf("BAD ARGUMENT TO JSON FILE: %s", argv[idx + 1]);
	goto cleanup;
cpp_dne:
	/* printf("BAD ARGUMENT TO CPP DIR: %s", argv[idx + 1]); */
	printf("NO PATH SUPPLIED TO CPP DIR");
	goto cleanup;
cleanup:
	if (api_json) {
		free(api_json);
	}
	if (cpp_dir) {
		free(cpp_dir);
	}
	return 1;
}
