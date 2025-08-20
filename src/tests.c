#include <unistd.h>
#include <assert.h>
#define LOG_FILE_HERE
#include "logger.h"
/* #include "scratch.h" */
#include "benchmark.h"


FILE* log_fp;


#define TESTS								\
	X(scratch) \
	X(base_protocol) \
	X(perfect_json)
	/* X(gdscript_parser) */
	/* X(json)									\ */
	/* X(gdscript_parser) */


#define X(A) extern int A##_test(int argc, char** argv);
TESTS
#undef X

int main(int argc, char** argv)
{
	int c;
	log_fp = fopen("test.log", "w");
	LOG("running unit tests");
	unsigned int i = 1;
	#define X(A) printf("\n[%d] Testing " #A ".c\n", i++); assert(A##_test(argc, argv) == 0); printf("A-OK: " #A ".c\n");
	TESTS
	#undef X
	LOG("finished unit tests");
	ENDLOG;
	return 0;
}
