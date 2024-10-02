#include <string.h>
#include <limits.h> /* PATH MAX */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "file_sys.h"
#include "tarragon.h"

#define CONFIG_JSON "tarragon.json"


void print_usage(void);

void print_version(void);

int generate_tarragon_json(char* dest, char* ext_api_path);

/*
 * IDEALIZED USER OPTIONS
 * -v
 *   prints version
 * -h
 *   prints usage
 * -g <path/to/extension_api.json>
 *   generates a tarragon.json configuration file
 */
int main(int argc, char** argv) {
	char cwd[PATH_MAX];
	if (getcwd(cwd, PATH_MAX) == NULL)
	{
		perror("Failed to get current working directory!");
		return 1;
	}
	for (int i = 1; i < argc && *argv[i] == '-'; i++) {
		switch (argv[i][1]) {
		case 'v':
			print_version();
			return 0;
		case 'h':
			print_usage();
			return 0;
		case 'g':
			if (argc == i + 1)
			{
				print_usage();
				fprintf(stderr, "INSUFFICIENT ARGUMENTS FOR '-g' OPTION!\n");
				perror("MUST PASS IN PATH TO extension_api.json TO USE FOR PROJECT!\n");
				return 1;
			}
			char* dest = join_path(cwd, CONFIG_JSON);
			int gen_res = generate_tarragon_json(dest, argv[i + 1]);
			free(dest);
			return gen_res;
		default:
			fprintf(stderr, "UNRECOGNIZED OPTION: '-%c'\n", argv[i][1]);
			print_usage();
			return 1;
		}
	}
	file_context context = {};
	if (find_upward(CONFIG_JSON, cwd, &context))
	{
		/* tarragon_t tarragon = {.project_root = cwd}; */
		/* init(context.file, &tarragon); */
		printf("Found %s in %s!\n", CONFIG_JSON, context.dir);
		char s[1024];
		while (fgets(s, sizeof(s), context.file) != NULL)
		{
			printf("%s", s);
		}
		fclose(context.file);
		free(context.dir);
		return 0;
	}
	perror("DID NOT FIND tarragon.json IN FILE HIEARCHY\n");
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
	return 1;
}

void print_usage(void)
{
	printf("tarragon blahblahblah GDScript LSP that uses standard IO instead of ports\n\n");
	printf("Usage\n\n");
	printf("  tarragon [options]\n\n");
	printf("If launched without any options, tarragon searches for 'tarragon.json' upwards in\n");
	printf("the file hierarchy, parses it for configuration, then acts as a Language Server.\n\n");
	printf("Options\n");
	printf("  -g <extension api> = generates a 'tarragon.json' file in the current working\n");
	printf("                       directory for project-wise configuration.\n");
	printf("                       - <extension api> path to the extension_api.json to use\n");
	printf("                         for a project.\n");
	printf("                         Call 'godot --dump-extension-api-with-docs' to create.\n");
	printf("  -h                 = Prints usage information.\n");
	printf("  -v                 = Prints version.\n");
}

void print_version(void)
{
	printf("0.0.0-BRUH");
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
