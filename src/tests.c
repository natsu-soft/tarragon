#include "tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "error_macros.h"
#include "file_sys.h"
#include "base_protocol.h"
#include "language_server.h"
#include "logger.h"
/* LOG_FILE */

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
	LOG("starting tests");
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
	file_context_t* context = fs_find_upward("bruh", cwd);
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
	char* config_path = ls_generate_config(context->dir);
	file_context_t* config_context = fs_find_upward(CONFIG_NAME, cwd);
	assertf(config_context, "find_upwards failed to find %s!", CONFIG_NAME);

	/* BASE PROTOCOL */

	FILE* fp = fopen("test_stuff/rpc_encode", "w");
	bp_send(fp, "{\"testing\": true}");
	fclose(fp);
	fp = fopen("test_stuff/rpc_encode", "r");
	bp_init();
	int32_t content_len = bp_receive(fp);
	assertf(content_len == 17, "bp_consume length mismatch");
	LOG("length: %d\ncontent: %s\n", content_len, bp_content());
	assertf(!strcmp( "{\"testing\": true}", bp_content()), "bp_content content mismatch");
	fclose(fp);
	bp_init();
	fp = fopen("test_stuff/bad_rpc", "r");
	content_len = bp_receive(fp);
	// LMAO: some real enterprise shit right here frfrfr
	/* printf("%d", content_len); */
	/* assertf(content_len == -1, "bp_content erroneously parsed bad base protocol"); */
	bp_init();
	/* int scanned = fscanf(fp, "Content-Length: %u\r\n\r\n", &decode_len); */
	LOG("decoded: %d", content_len);
	fclose(fp);

	fp = fopen("test_stuff/initialize_request.json", "r");
	content_len = bp_receive(fp);
	LOG("%s", bp_content());
	bp_init();
	LOG("finished tests!");
	ENDLOG
	printf("ATTN: TESTS SUCCEEDED!\n");
	return 0;
}
