#include <unistd.h>
#include <assert.h>
#define LOG_FILE_HERE
#include "logger.h"
/* #include "scratch.h" */
#include "benchmark.h"

FILE* log_fp;


#define TESTS								\
	X(linked_list) \
	X(godot_api) \
	X(gdscript_suite) \
	/* X(perfect_json) \ */
	/* X(xml) \ */
	/* X(base_protocol) \ */
	// declare pass
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
