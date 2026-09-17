#include <unistd.h>
#include <assert.h>
#include "job.h"
#define LOG_FILE_HERE
#include "logger.h"
#undef LOG_FILE_HERE
#include "allocators/scratch.h"
#include "allocators/workpad.h"
#include "gdscript/lex.h"
#include "gdscript/flexer.h"
#include "benchmark.h"
#include "threading.h"



#define SRC_TESTS								\
	X(threading) \
	X(jobs) \
	/* X(linked_list) \ */
	/* X(godot_api) \ */
	/* X(gdscript_suite) \ */
	/* X(perfect_json) \ */
	/* X(xml) \ */
	/* X(base_protocol) \ */
	// declare pass
	/* X(json)									\ */
	/* X(gdscript_parser) */


#define X(A) extern int A##_test(int argc, char** argv);
SRC_TESTS
#undef X


static void job_nop(void* args, struct thread_context* ctx)
{
	/* int* iteration = args; */
	int32_t iteration = (int32_t)args;
	printf("%d\n", iteration);
	fflush(stdout);
}

int jobs_test(int argc, char** argv)
{
	#define JT_NUM 1000000
	printf("testing jobs N=%d\n", JT_NUM);
	CLOCKTIME(
		for (int32_t i = 0; i < JT_NUM; ++i)
		{
			jobs_add(job_nop, (void*)i);
		}
		jobs_sync();
	)
	CLOCKTIME(
		for (int32_t i = 0; i < JT_NUM; ++i)
		{
			job_nop((void*)i, NULL);
		}
	)
	return 0;
}

static void print_char(char c, struct thread_context* ctx)
{
	printf("wrkr%d: %c\n", ctx->thread_num, c);
	fflush(stdout);;;
}

int main(int argc, char** argv)
{
	printf("[ SANITY ] testing barrier_wait() with 256 threads");
	assert(threading_test(argc, argv) == 0);
	printf("A-OK: barrier_wait()\n");
	printf("\n[ SANITY ] atom_ring::synching 1024 times\n"
		"  nota bene: I'm using jobs_sync() but the real culprit is read locking fuckily\n");
	jobs_init(6, 4096);
	/* MICROTIME(jobs_kill();) */
	/* return 0; */
	char spinner[4] = { '-', '\\', '|', '/' };
	uint32_t spin_idx = 0;
	for (int i = 0; i < 64; ++i)
	{
		MICROTIME(jobs_sync();)
	}
	fflush(stdout);
	MICROTIME(jobs_finish();)
	return 0;
	jobs_add_initializer(scratch_init);
	printf("queued scratch_init()");
	jobs_add_initializer(workpad_init);
	printf("queued workpad_init()");
	jobs_add_initializer(lex_init);
	printf("queued lex_init()");
	jobs_add_initializer(flexer_init);
	printf("queued flexer_init()");
	fflush(stdout);
	/* for (char c = ' '; c < '~'; ++c) */
	/* { */
	/* 	jobs_add(print_char, c); */
	/* } */
	MICROTIME(jobs_sync();)
	printf("POST-INITIALIZER SYNCH\n");
	fflush(stdout);
	jobs_finish();
	return 0;
	/* MICROTIME( */
	/* scratch_init(); */
	/* workpad_init(); */
	/* lex_init(); */
	/* flexer_init();) */
	/* jobs_kill(); */
	log_fp = fopen("test.log", "w");
	LOG("running unit tests");
	unsigned int i = 1;
	#define X(A) printf("\n[%d] Testing " #A ".c\n", i++); assert(A##_test(argc, argv) == 0); printf("A-OK: " #A ".c\n");
	SRC_TESTS
	#undef X
	LOG("finished unit tests");
	ENDLOG;
	printf("\n[CLEANUP] killing job threads\n");
	/* jobs_sleep(); */
	jobs_finalize();
	printf("ATTN: ALL TESTS PASSED!\n");
	return 0;
}
