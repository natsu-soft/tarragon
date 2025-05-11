#include <unistd.h>
#include <assert.h>
#include "logger.h"
#include "scratch.h"
#include "benchmark.h"


/* #define UNIT_TESTS \ */
/* 	TEST(file_sys) \ */
/* 	TEST(mem_sys) \ */
/* 	TEST(base_protocol) \ */
/* 	TEST(json) \ */
/* 	TEST(array_trie) */

/* #define UNIT_TESTS \ */
/* 	TEST(array_trie) */
/* 	/\* TEST(free_list) \ *\/ */
/* 	/\* TEST(free_tree) *\/ */
/* 	/\* TEST(free_list) *\/ */
/* 	/\* TEST(free_tree) \ *\/ */
/* 	/\* TEST(free_list) \ *\/ */
/* 	/\* TEST(array_trie) *\/ */

#define UNIT_TESTS								\
	TEST(json)									\
	TEST(gdscript_parser)

#define TEST(A) extern int A##_test(int argc, char** argv);
UNIT_TESTS
#undef TEST

int main(int argc, char** argv)
{
	int c;
	log_fp = NULL;
	while ((c = getopt(argc, argv, "l:")) != -1)
	{
		switch (c)
		{
		case 'l':
			log_fp = fopen(optarg , "w");
			LOG("opened tarragon.log for logging");
			break;
		}
	}
	unsigned int i = 1;
	#define TEST(A) printf("\n[%d] Testing " #A ".c\n", i++); assert(A##_test(argc, argv) == 0); printf("A-OK: " #A ".c\n");
	UNIT_TESTS;
	#undef TEST
	ENDLOG;
	scratch_audit();
	return 0;
}
