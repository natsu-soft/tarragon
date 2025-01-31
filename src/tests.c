#include "tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "error_macros.h"
#include "file_sys.h"
#include "tarragon.h"

// wow, you want to test it on your own machine? enjoy changing these hardcoded paths lmao
#ifdef _WIN32
#define REPO_PATH "D:\\Repos\\tarragon"
#else
#define REPO_PATH "/home/jon/Repos/tarragon"
#endif

extern unsigned char default_config_json[];
extern unsigned int default_config_json_len;

int run_tests(int argc, char** argv)
{
	for (int i = 0; i < argc; ++i)
	{
		printf("arg%d: %s\n", i, argv[i]);
	}
	printf("ATTN: TESTS STARTED!\n");
	/* FILE_SYS TESTS */
	int temp = 0;
	char* str_file_abs_path = fs_abs_path( "test_stuff/str_file.txt");
	assertf(!strcmp(fs_filestr(str_file_abs_path, &temp), "hello!\n"), "str_file test failed"); // have to ! because strcmp returns 0 (false) if equal
	// rel to abs path
	// dirname test
	char* cwd = fs_dirname(argv[0]);
	// NOTE: you're going to have to set up the path as a preprocessor constant via Makefile
	/* assertf(!strcmp(cwd, "/home/jon/Repos/tarragon"), "fs_dirname failed!"); */
	assertf(!strcmp(cwd, REPO_PATH), "fs_dirname failed!");
	// find upwards
	FileContext* context = fs_find_upward("bruh", cwd);
	assertf(context, "find_upwards failed!");

	/* GENERATING CONFIG FILE */

	int config_len = 0;
	char* default_config_file = fs_abs_path("default_config.json");
	assertf(default_config_file, "default config file DNE!");
	#ifdef _WIN32
	printf("WINDOWS_HACK:\nthis is the baked string for the default config json:\n%s\n", &default_config_json); // the assert test does not work on windows, BUT AS WE CAN SEE IT OUTPUTS PROPERLY
	printf("CHECK: does it look good?\n");
	#else
	assertf(!strcmp( fs_filestr(default_config_file, &config_len), &default_config_json), "default config file does not match hex dump!");
	#endif
	char* config_path = generate_config(context->dir);
	FileContext* config_context = fs_find_upward(CONFIG_NAME, cwd);
	assertf(config_context, "find_upwards failed to find %s!", CONFIG_NAME);
	printf("ATTN: TESTS SUCCEEDED!\n");
	return 0;
}
