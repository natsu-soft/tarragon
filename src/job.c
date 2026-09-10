#include "job.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include "buffer_type.h"
#include "spinlock_types.h"
#include "allocators/arena.h"
#include "memery.h"

#define JOB_ARG_CAP 16384

struct job
{
	job_func job_fptr;
	int32_t arg_size;
};

struct
{
	void* buffer;
	int32_t size;
	int fd; // note this gets closed automagically once all references dropped SO YOU HAVE TO KEEP THE FD
	// NOTE: head MUST be atomic, but I don't think tail has to be :^)
	_Atomic(int32_t) head;
	_Atomic(int32_t) tail;
	// NOTE: head is atomic so that way job_add works freely
} job_ring = {0};


static pthread_mutex_t sleep_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sleep_cnd = PTHREAD_COND_INITIALIZER;
static _Atomic(enum job_state) job_state = JOBS_SLEEP;
static int32_t num_job_threads = 0;
static spinlock job_spinlock;
static virtual_buffer job_vbuff;
static pthread_t* job_threads;

// TODO: align64
static void job_ring_write(void* src, const int32_t size)
{
	const int32_t head0 = atomic_load(&job_ring.head);
	const int32_t tail0 = atomic_load(&job_ring.tail);
	assert(tail0 < head0 ? head0 + size < tail0 + job_ring.size : head0 + size < tail0);
	memcpy(job_ring.buffer + head0, src, size);
	atomic_store(&job_ring.head, head0 + size);
}

// TODO: align64
static void job_ring_read(void* dest, const int32_t size)
{
	const int32_t head0 = atomic_load(&job_ring.head);
	const int32_t tail0 = atomic_load(&job_ring.tail);
	assert(tail0 < head0 ? tail0 + size < head0 : tail0 + size < head0 + job_ring.size);
	memcpy(dest, job_ring.buffer + tail0, size);
}

static void job_thread_main(struct thread_context* ctx)
{
	while (job_state != JOBS_KILL)
	{
		if (job_state == JOBS_WORK)
		{
			while (atomic_fetch_or(&job_spinlock, SPIN_LOCK)) { ; }
			if (job_ring.head == job_ring.tail) { continue; }
			struct job job;
			job_ring_read(&job, sizeof job);
			assert(job.arg_size <= JOB_ARG_CAP && "job arguments greater than 16KB!");
			if (job.arg_size > 0)
			{
				job_ring_read(ctx->args, job.arg_size);
			}
			atomic_store(&job_spinlock, SPIN_UNLOCK);
			ctx->scratch.size = 0;
			job.job_fptr(ctx->args, ctx);
		}
		else if (job_state == JOBS_SLEEP)// lock threads & do no work
		{
			pthread_mutex_lock(&sleep_mtx);
			pthread_cond_wait(&sleep_cnd, &sleep_mtx);
			pthread_mutex_unlock(&sleep_mtx);
		}
	}
}

void jobs_init(const int32_t num_threads, const int32_t job_ring_size)
{
	ring_map((struct ring_buffer*)&job_ring, MB, "jobring");
	job_vbuff = memalloc(PAGE_SIZE + (64 * KB + JOB_ARG_CAP) * num_threads);
	arena_t thread_setup_arena =
	{
		.allocation = job_vbuff.addr,
		.cap = KB
	};
	arena_t thread_mem_arena =
	{
		.allocation = (uint8_t*)job_vbuff.addr + PAGE_SIZE,
		.cap = 80 * KB * num_threads
	};
	job_threads = arena_calloc(&thread_setup_arena, num_threads * sizeof *job_threads);
	num_job_threads = num_threads;
	job_spinlock = SPIN_UNLOCK;
	job_state = JOBS_SLEEP;
	pthread_attr_t attr = {0};
	for (int32_t i = 0; i < num_threads; ++i)
	{
		struct thread_context* ctx = arena_alloc_aligned(&thread_setup_arena, sizeof *ctx, 64);
		*ctx = (struct thread_context)
		{
			.num = i,
			.mask = 1 << i,
			.args = arena_alloc(&thread_mem_arena, JOB_ARG_CAP),
			.scratch =
			{
				.allocation = arena_alloc(&thread_mem_arena, 64 * KB),
				.cap = 64 * KB
			},
		};
		pthread_create(job_threads + i, &attr, job_thread_main, ctx);
	}
}

void __jobs_add_noarg(job_func job)
{
	struct job j = { .job_fptr = job };
	job_ring_write(&j, sizeof j);
}

void __jobs_add_arg(job_func job, struct buffer args)
{
	struct job j = { .job_fptr = job, .arg_size = args.size };
	job_ring_write(&j, sizeof j);
	job_ring_write(args.data, args.size);
}

static void wait_thread(void* args, struct thread_context* ctx)
{
	_Atomic(uint32_t) *wait_bitmask = args;
	atomic_store(wait_bitmask, *wait_bitmask ^ ctx->mask);
	while (atomic_load(wait_bitmask)) { ; }
}

void jobs_wait_all(void)
{
	_Atomic(uint32_t) wait_bitmask = 0;
	for (int32_t i = 0; i < num_job_threads; ++i)
	{
		wait_bitmask |= 1 << i;
	}
	for (int32_t i = 0; i < num_job_threads; ++i)
	{
		jobs_add(wait_thread, (struct buffer){ .data = &wait_bitmask, .size = sizeof wait_bitmask });
	}
	if (job_state != JOBS_WORK)
	{
		jobs_wake();
	}
	while (atomic_load(&wait_bitmask)) { ; }
}


void jobs_sleep(void)
{
	job_state = JOBS_SLEEP;
}

void jobs_wake(void)
{
	// gotta do this dance to make sure all threads are waiting for sleep_cnd
	job_state = JOBS_WORK;
	pthread_mutex_lock(&sleep_mtx);
	pthread_cond_broadcast(&sleep_cnd);
	pthread_mutex_unlock(&sleep_mtx);
}

void jobs_kill(void)
{
	job_state = JOBS_KILL;
	pthread_mutex_lock(&sleep_mtx);
	pthread_cond_broadcast(&sleep_cnd);
	pthread_mutex_unlock(&sleep_mtx);
	for (int32_t i = 0; i < num_job_threads; ++i)
	{
		pthread_join(job_threads[i], NULL);
	}
	memfree(&job_vbuff);
	ring_unmap((ring_buffer*)&job_ring);
}
