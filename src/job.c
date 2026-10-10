// NOTE: futex is better but CHRIST fuck barriers & shit fam
#include "job.h"
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include "atom_ring.h"
/* #include "safe_ring.h" */
#include "allocators/arena.h"
#include "allocators/scratch.h"
#include "memery.h"
#include "threading.h"


// TODO: FUCK spinlocks FUCK mutex USE futexes
// https://martinuke0.github.io/posts/2026-05-17-building-userland-mutexes-with-the-futex-system-call/
// NOTE: https://h4x0r.org/futex/
enum
{
	REGULAR_JOB = 0x00,
	INITIALIZER = 0x01,
	KILL_WORKER = 0x02
};

enum jobs_state
{
	JOBS_ALIVE	= 0x00,
	JOBS_DEAD	= 0x01,
};

struct job
{
	union
	{
		struct
		{
			unsigned long dont_touch : 32;
			unsigned int pls_dont_touch_also: 25;
			uint8_t tag : 7;
		};
		uint64_t bits;
		void* fptr;
	};
	void* args;
};

static _Atomic(enum jobs_state) jobs_state = JOBS_ALIVE;
static struct atom_ring job_ring = {0};
static int32_t num_worker_threads = 0;
static virtual_buffer job_vbuff;
static pthread_t* worker_threads;

static void worker_main(struct thread_context* ctx)
{
	int32_t res;
	struct job job;
	while (atomic_load_explicit(&jobs_state, memory_order_acquire) == JOBS_ALIVE)
	{
		/* printf("%d-%d\n", ctx->sync_epoch, ctx->thread_num); */
		/* fflush(stdout); */
		res = atom_ring_concurrent_read(&job_ring, &job, sizeof job, NULL);
		/* res = atom_ring_read(&job_ring, &job, sizeof job); */
		if (res == sizeof job)
		{
			/* if (job.fptr == NULL) */
			/* { */
			/* 	printf("NULLPTR:wrkr%d:{ %0x, %0x }\n", ctx->thread_num, job.fptr, job.args); */
			/* 	fflush(stdout); */
			/* 	assert(job.fptr != NULL); */
			/* } */
			/* else if (job.tag == REGULAR_JOB) */
			if (job.tag == REGULAR_JOB)
			{
				job_func job_function = job.fptr;
				job_function(job.args, ctx);
			}
			else if (job.tag == INITIALIZER)
			{
				job.tag = 0x00;
				init_func initializer = job.fptr;
				initializer();
			}
		}
		else if (res != ATOM_RING_EVICTED)
		{
			switch(res)
			{
				// TODO: sync_addr error handing
			default: assert(res && "atomic_ring_read() returned a sync_addr failure");
			}
		}
	}
	return;
}

void jobs_init(const int32_t num_threads, const int32_t min_job_ring_size)
{
	ring_map((struct ring_buffer*)&job_ring, min_job_ring_size, "jobring");
	assert(job_ring.size == (1 << __builtin_ctz(job_ring.size)));
	/* __uint128_t* ring_test =  job_ring.addr; */
	/* *ring_test = 0xB16B00B5B00BCAFE; */
	/* assert(strncmp(job_ring.addr, job_ring.addr + job_ring.size, 16) == 0); */
	assert(((uint64_t)job_ring.addr & 0x3f) == 0x00);
	/* assert(((uint64_t)&job_ring.read_state & 0x03) == 0); */
	/* assert(((uint64_t)&job_ring.write_state & 0x03) == 0); */
	/* memset(job_ring.addr, 0x00, 2 * job_ring.size); */
	atomic_thread_fence(memory_order_release);
	job_vbuff = memalloc(PAGE_SIZE + 64 * KB * num_threads);
	arena_t thread_setup_arena =
	{
		.addr = job_vbuff.addr,
		.cap = PAGE_SIZE
	};
	arena_t thread_mem_arena =
	{
		.addr = (uint8_t*)job_vbuff.addr + PAGE_SIZE,
		.cap = 64 * KB * num_threads
	};
	worker_threads = arena_calloc(&thread_setup_arena, num_threads * sizeof *worker_threads);
	num_worker_threads = num_threads;
	// set the schedule policy to PREVENT SLEEPING
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	for (int32_t i = 0; i < num_threads; ++i)
	{
		struct thread_context* ctx = arena_alloc_aligned(&thread_setup_arena, sizeof *ctx, 64);
		*ctx = (struct thread_context)
		{
			.thread_num = i,
			.thread_mask = 1 << i,
			.scratch =
			{
				.addr = arena_alloc(&thread_mem_arena, 64 * KB),
				.cap = 64 * KB
			},
		};
		pthread_create(worker_threads + i, &attr, worker_main, ctx);
	}
}

void jobs_add_initializer(init_func initializer)
{
	struct job j = { .fptr = initializer };
	j.tag = INITIALIZER;
	atom_ring_concurrent_write(&job_ring, &j, sizeof j, NULL);
}

void jobs_add_noarg(job_func job)
{
	struct job j = { .fptr = job };
	atom_ring_concurrent_write(&job_ring, &j, sizeof(j), NULL);
}

void jobs_add_arg(job_func job, void* args)
{
	struct job j = { .fptr = job, .args = args };
	atom_ring_concurrent_write(&job_ring, &j, sizeof(j), NULL);
}

static inline void worker_sync(struct barrier* barrier, struct thread_context* ctx)
{
	barrier_wait(barrier);
	ctx->sync_epoch++;
}

// IF YOU CRASH THAT'S A FEATURE
void jobs_sync()
{
	struct barrier sync_barrier;
	barrier_init(&sync_barrier, num_worker_threads + 1);
	struct job sync = { .fptr = worker_sync, .args = &sync_barrier };
	atom_ring_n_write(&job_ring, &sync, num_worker_threads, sizeof sync);
	barrier_wait(&sync_barrier);
	fflush(stdout);
}

void jobs_finish(void)
{
	atomic_store_explicit(&jobs_state, JOBS_DEAD, memory_order_release);
	// you need to write data so that way threads waiting to READ actually READ & EXIT
	// spiritually it should be zero
	/* struct job kill_job = { .tag = KILL_WORKER }; */
	/* atom_ring_n_write(&job_ring, &kill_job,  num_worker_threads, sizeof kill_job); */
	atom_ring_evict_all(&job_ring);
	for (int32_t i = 0; i < num_worker_threads; ++i)
	{
		pthread_join(worker_threads[i], NULL);
		fflush(stdout);
	}
	memfree(&job_vbuff);
	ring_unmap((ring_buffer*)&job_ring);
}

struct _test_struct
{
	uint32_t* ints;
	uint32_t idx;
};

static void worker_test(struct _test_struct* args, struct thread_context* ctx)
{
	args->ints[args->idx] ^= args->idx;
}

#include "benchmark.h"
static char spinner[4] = { '-', '\\', '|', '/' };
int jobs_test(char** argc, int argv)
{
	#define TEST_LEN 1048576
	int32_t test_ints[TEST_LEN] = {0};
	// dumbass, you need to allocate heap memory for this
	struct _test_struct* worker_args = scratch_alloc(TEST_LEN * sizeof *worker_args);
	printf("- assigning indices into array of size %d", TEST_LEN);
	fflush(stdout);
	CLOCKMESSAGE("", for (uint32_t i = 0; i < TEST_LEN; ++i)
	{
		worker_args[i] = (struct _test_struct){ .ints = test_ints, .idx = i};
		jobs_add(worker_test, worker_args + i);
	}
	// xor a MASSIVE bit buffer EXACTLY ONCE.
	// then sync
	jobs_sync();)
	fflush(stdout);
	uint8_t check = 0;
	assert((0x01 ^ 0x01) == 0x00);
	assert((0x01 ^ 0x01 ^ 0x01) != 0x00);
	uint8_t res = 0;
	for (int32_t i = 0; i < TEST_LEN; ++i)
	{
		if (test_ints[i] != i)
		{
			res = 1;
			printf("%d != %d\n", i, test_ints[i]);
		}
	}
	return res;
}
