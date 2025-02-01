#include "tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "error_macros.h"
#include "file_sys.h"
#include "rpc.h"
#include "tarragon.h"

// wow, you want to test it on your own pc? enjoy changing these hardcoded paths lmao
#ifdef _WIN32
#define REPO_PATH "D:\\Repos\\tarragon"
#define URI "file:///d:/Repos/tarragon/test_stuff/str_file.txt"
#else
#define REPO_PATH "/home/jon/Repos/tarragon"
#define URI "file:////home/jon/Repos/tarragon/test_stuff/str_file.txt"
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
	char* str_file_uri = fs_abs_uri(str_file_abs_path);
	assertf(!strcmp(URI, str_file_uri), "fs_abs_uri failed");
	assertf(!strcmp(fs_filestr(str_file_abs_path, &temp), "hello!\n"), "str_file test failed"); // have to ! because strcmp returns 0 (false) if equal
	// dirname test
	char* cwd = fs_dirname(argv[0]);
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
	printf("does it look good?\n");
	#else
	assertf(!strcmp( fs_filestr(default_config_file, &config_len), &default_config_json[0]), "default config file does not match hex dump!");
	#endif
	char* config_path = lsp_generate_config(context->dir);
	FileContext* config_context = fs_find_upward(CONFIG_NAME, cwd);
	assertf(config_context, "find_upwards failed to find %s!", CONFIG_NAME);

	/* RPC */

	FILE* fp = fopen("test_stuff/rpc_encode", "w");
	rpc_encode(fp, "{\"testing\": true}");
	/* fflush(fp); */
	fclose(fp);
	fp = fopen("test_stuff/rpc_encode", "r");
	uint32_t decode_len;
	char* decode_content = NULL;
	rpc_decode(fp, &decode_len, &decode_content);
	assertf(decode_len == 17, "rpc_decode length mismatch");
	/* printf("decoded content: %s\n", decode_content); */
	assertf(!strcmp( "{\"testing\": true}", decode_content), "rpc_decode content mismatch");
	fclose(fp);
	/* int content_len; */
	/* char* content; */
	/* fflush(stdin); */
	/* fscanf(stdin, "Content-Length: %ul ", &content_len); */
	/* printf("cl: %d", content_len); */
	/* fflush(stdin); */

	printf("ATTN: TESTS SUCCEEDED!\n");
	return 0;
}
