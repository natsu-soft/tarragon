#include <unistd.h>
#include <assert.h>
#define LOG_FILE_HERE
#include "logger.h"
/* #include "scratch.h" */
#include "benchmark.h"


FILE* log_fp;


#define UNIT_TESTS								\
	TEST(scratch) \
	TEST(base_protocol) \
		TEST(json) \
	/* TEST(json)									\ */
	/* TEST(gdscript_parser) */

#define TEST(A) extern int A##_test(int argc, char** argv);
UNIT_TESTS
#undef TEST

int main(int argc, char** argv)
{
	int c;
	log_fp = fopen("test.log", "w");
	LOG("running unit tests");
	unsigned int i = 1;
	#define TEST(A) printf("\n[%d] Testing " #A ".c\n", i++); assert(A##_test(argc, argv) == 0); printf("A-OK: " #A ".c\n");
	UNIT_TESTS
	#undef TEST
	LOG("finished unit tests");
	ENDLOG;
	return 0;
}
