#include <unistd.h>
#include <assert.h>
#include "job.h"
#define LOG_FILE_HERE
#include "logger.h"
#undef LOG_FILE_HERE
#include "stdout_ring.h"
#include "allocators/scratch.h"
#include "allocators/workpad.h"
#include "gdscript/lex.h"
#include "gdscript/flexer.h"
#include "benchmark.h"

#define SAN_CHECK \
	X(barrier_wait)								\
	/* X(atom_ring) */

#define SRC_TESTS								\
	X(jobs) \
	X(linked_list) \
	X(perfect_json) \
	X(godot_api) \
	X(gdscript_suite) \
	/* X(xml) \ */
	/* X(base_protocol) \ */
	// declare pass
	/* X(json)									\ */
	/* X(gdscript_parser) */


#define X(A) extern int A##_test(int argc, char** argv);
SAN_CHECK
SRC_TESTS
#undef X

int main(int argc, char** argv)
{
	#define X(A) printf("\n[ SANCHECK ] Checking " #A "()\n"); assert(A##_sancheck() == 0); printf("SANE: " #A "()\n");
	SAN_CHECK
	#undef X
	jobs_init(4, 4096);
	printf("worker threads initialized. Initializing other systems\n");
	fflush(stdout);
	MICROMESSAGE("INITS", jobs_add_initializer(stdout_ring_init);
	jobs_add_initializer(scratch_init);
	jobs_add_initializer(workpad_init);
	jobs_add_initializer(lex_init);
	jobs_add_initializer(flexer_init); /* TODO: DELETE THIS */
	jobs_sync();)
	printf("\nrunning tests\n");
	fflush(stdout);
	log_fp = fopen("test.log", "w");
	LOG("running unit tests");
	unsigned int i = 1;
	#define X(A) \
		printf("\n[%d] Testing " #A ".c\n", i++); \
		assert(A##_test(argc, argv) == 0 && "TEST-FAILED!"); \
		printf("A-OK: " #A ".c\n"); \
		fflush(stdout);
	SRC_TESTS
	#undef X
	LOG("finished unit tests");
	ENDLOG;
	printf("\n[CLEANUP] killing job threads\n");
	/* jobs_sleep(); */
	jobs_finish();
	printf("A-OK: all threads killed\n");
	printf("\nATTN: ALL TESTS PASSED!\n");
	return 0;
}
