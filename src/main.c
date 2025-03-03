#include <string.h>
#include <limits.h> /* PATH MAX */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "file_sys.h"
#include "error_macros.h"
#include "language_server.h"
/* #include "tests.h" */
#include "logger.h"
#include "token_trie.h"

#define CONFIG_JSON "tarragon.json"

extern char supported_godot[];

void print_usage(void)
{
	printf("tarragon blahblahblah GDScript LSP that uses standard IO instead of ports\n\n");
	printf("Usage\n\n");
	printf("  tarragon [options]\n\n");
	printf("If launched without any options, tarragon searches for 'tarragon.json' upwards in\n");
	printf("the file hierarchy, parses it for configuration, then acts as a Language Server.\n\n");
	printf("Options\n");
	printf("  -h                 = Prints usage information.\n");
	printf("  -v                 = Prints tarragon version.\n");
	printf("  -s                 = Prints supported Godot version.\n");
	/* printf("  -g                 = Prints supported Godot version.\n"); */
}

void print_version(void)
{
	printf("0.0.0-LMAO\n");
}

void print_supported_godot(void)
{
	printf("%s", &supported_godot[0]);
}

int generate_tarragon_json(char* dest, char* ext_api_path)
{
	FILE* file = fopen(dest, "w");
	if (!file)
	{
		fprintf(stderr, "FAILED TO OPEN FILE '%s' FOR WRITING\n", dest);
		print_usage();
		return 1;
	}
	fprintf(file, "{\n\t\"extension_api\": \"%s\",\n", ext_api_path);
	fputs("\t\"warnings\": [],\n\t\"ignored_dirs\": [],\n}", file);
	fclose(file);
	return 0;
}

int run_tests(int argc, char** argv);

/*
 * IDEALIZED USER OPTIONS
 * -h
 *   prints usage
 * -v
 *   prints tartragon version
 * -s
 *   prints supported Godot version
 */
int main(int argc, char** argv) {
	int c;
	while ((c = getopt(argc, argv, "l:tvhs")) != -1)
	{
		/* printf("opt: %c\n", c); */
		switch(c)
		{
		case 'l':
			log_fp = fopen(optarg , "w");
			LOG("opened tarragon.log for logging");
			break;
		case 't':
			return run_tests(argc, argv);
		case 'v':
			print_version();
			return 0;
		case 'h':
			print_usage();
			return 0;
		case 's':
			print_supported_godot();
			return 0;
		case ':':
			log_error("Unrecognized option: %s", argv[optind - 1]);
			return 1;
		case '?':
			log_error("Unrecognized option: %s", argv[optind - 1]);
			return 1;
		}
	}
	/* trie_tests(stdout); */
	return 0;
	LOG("initializing tarragon");
	/* setbuf(stdin, NULL); */
	/* setbuf(stdout, NULL); */
	return ls_listen(stdin, stdout);
	/* echo(100, stdin, log_fp); */

	/* bp_receive(stdin); */

	/* while (c = getc(stdin), c) */
	/* { */
	/* 	PLAIN_LOG("%c", c); */
	/* } */
	LOG("exiting");
	ENDLOG
	return 0;
	char cwd[PATH_MAX];
	if (getcwd(cwd, PATH_MAX) == NULL)
	{
		perror("Failed to get current working directory!");
		/* exit(EXIT_FAILURE); */
	}
	// search for project.godot upwards
	// look for tarragon.json in found dir
	// if DNE then generate the default
	// init tarragon using the parsed config
	/* for (int i = 1; i < argc && *argv[i] == '-'; i++) { */
	/* 	switch (argv[i][1]) { */
	/* 	case 't': */
	/* 		run_tests(); */
	/* 		return 0; */
	/* 	case 'v': */
	/* 		print_version(); */
	/* 		return 0; */
	/* 	case 'h': */
	/* 		print_usage(); */
	/* 		return 0; */
	/* 	case 'g': */
	/* 		print_supported_godot(); */
	/* 	default: */
	/* 		fprintf(stderr, "UNRECOGNIZED OPTION: '-%c'\n", argv[i][1]); */
	/* 		print_usage(); */
	/* 		exit(EXIT_FAILURE); */
	/* 	} */
	/* } */
	/* FileContext context = {}; */
	/* if (fsfind_upward(CONFIG_JSON, cwd, &context)) */
	/* { */
	/* 	tarragon_t tarragon = {.project_root = cwd}; */
	/* 	/\* init(context.file, &tarragon); *\/ */
	/* 	printf("Found %s in %s!\n", CONFIG_JSON, context.dir); */
	/* 	init(context.fp, &tarragon); */
	/* 	// free(context.dir); // WHY? Because it lives on the stack */
	/* 	exit(EXIT_SUCCESS); */
	/* } */
	perror("DID NOT FIND tarragon.json IN FILE HIEARCHY\n");
	return 1;
	/* exit(EXIT_FAILURE); */
	/* printf("argv[0]: %s\n", argv[0]); */
	/* find_file_t res = { .file=NULL, .dir=NULL }; */
	/* char* dir = dirname(argv[0]); */
	/* FILE* f = find_upward("foo.txt", dir); */
	/* if (find_upward(CONFIG_JSON, dir, &res)) */
	/* { */
	/* 	printf("FOUND FILE in %s!", res.dir); */
	/* 	fclose(res.file); */
	/* 	free(res.dir); */
	/* } */
	/* else */
	/* { */
	/* 	fprintf(stderr, "did not find %s :(", CONFIG_JSON); */
	/* } */
	/* free(dir); */
}

/***********/
/* TESTING */
/***********/

#define UNIT_TESTS \
TEST(file_sys) \
TEST(language_server) \
TEST(base_protocol) \
/* TEST(unzip) */ \
TEST(token_trie)

#define TEST(A) extern int A##_test(int argc, char** argv);
UNIT_TESTS
#undef TEST
#define TEST(A) assert(A##_test(argc, argv) == 0);

int run_tests(int argc, char** argv)
{
	UNIT_TESTS;
	ENDLOG;
	return 0;
}
