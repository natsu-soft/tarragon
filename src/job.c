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
#include "allocators/arena.h"
#include "memery.h"
#include "threading.h"
#include "allocators/scratch.h"


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
		res = atom_ring_read(&job_ring, &job, sizeof job);
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
		else
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
	__uint128_t* ring_test =  job_ring.addr;
	*ring_test = 0xB16B00B5B00BCAFE;
	assert(strncmp(job_ring.addr, job_ring.addr + job_ring.size, 16) == 0);
	assert(((uint64_t)&job_ring.read_state & 0x03) == 0);
	assert(((uint64_t)&job_ring.write_state & 0x03) == 0);
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
	atom_ring_write(&job_ring, &j, sizeof j);
}

void jobs_add_noarg(job_func job)
{
	struct job j = { .fptr = job };
	atom_ring_write(&job_ring, &j, sizeof(j));
}

void jobs_add_arg(job_func job, void* args)
{
	struct job j = { .fptr = job, .args = args };
	atom_ring_write(&job_ring, &j, sizeof(j));
}

static inline void worker_sync(struct barrier* barrier, struct thread_context* ctx)
{
	barrier_wait(barrier);
	ctx->sync_epoch++;
}

static struct barrier sync_barriers[16] = {0};
static uint8_t barrier_idx = 0;

void jobs_sync()
{
	/* sync_barriers[barrier_idx].count = num_worker_threads + 1; */
	/* usleep(1024); // HILARIOUS BUT NEEDED */
	barrier_init(sync_barriers + barrier_idx, num_worker_threads + 1);
	struct job sync = { .fptr = worker_sync, .args = sync_barriers + barrier_idx };
	atom_ring_nwrite(&job_ring, &sync, num_worker_threads, sizeof sync);
	barrier_wait(sync_barriers + barrier_idx);
	// NOTE: magic barrier index toggle that makes the atom ring work
	//       yadda-yadda stack addresses yadda-yadda "reading too fast"
	//       EVEN THO I AM ACTUALLY ATOMICALL INITIALIZING IT WTF????
	/* barrier_idx = (barrier_idx + 1) & 0x01; */
	/* barrier_idx = (barrier_idx + 1) & 0x03; */
	barrier_idx = (barrier_idx + 1) & 0x0f;
}

void jobs_finish(void)
{
	printf("> jobs_finish():finishing last \"active\" jobs then joining threads\n");
	fflush(stdout);
	atomic_store_explicit(&jobs_state, JOBS_DEAD, memory_order_release);
	// you need to write data so that way threads waiting to READ actually READ & EXIT
	// spiritually it should be zero
	struct job kill_job = { .tag = KILL_WORKER };
	atom_ring_nwrite(&job_ring, &kill_job,  num_worker_threads, sizeof kill_job);
	for (int32_t i = 0; i < num_worker_threads; ++i)
	{
		pthread_join(worker_threads[i], NULL);
		fflush(stdout);
	}
	memfree(&job_vbuff);
	ring_unmap((ring_buffer*)&job_ring);
}
