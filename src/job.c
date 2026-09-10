#include "job.h"
/* #include <stdio.h> */
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include "buffer_type.h"
#include "spinlock_types.h"
#include "allocators/arena.h"
#include "memery.h"

#define JOB_ARG_CAP 16384

enum
{
	NON_INITIALIZER = 0x00,
	INITIALZIER = 0x7f
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
	int32_t arg_size;
};

struct
{
	void* buffer;
	int32_t size;
	int fd; // note this gets closed automagically once all references dropped SO YOU HAVE TO KEEP THE FD
	// NOTE: head MUST be atomic, but I don't think tail has to be :^)
	_Atomic(int32_t) head;
	int32_t tail;
	_Atomic(int32_t) used;
	_Atomic(int32_t) free;
	// NOTE: head is atomic so that way job_add works freely
} job_ring = {0};


static pthread_barrier_t sleep_barrier;
static pthread_mutex_t sleep_mtx = PTHREAD_MUTEX_INITIALIZER;
static _Atomic(enum job_state) job_state = JOBS_SLEEP;
static int32_t num_job_threads = 0;
static spinlock job_spinlock;
static virtual_buffer job_vbuff;
static pthread_t* job_threads;

// NOTE: dont give a FUCK about alignment since we memcpy gang
static void job_ring_write(void* src, const int32_t size)
{
	assert(size <= job_ring.free);
	const int32_t head = atomic_load(&job_ring.head);
	memcpy((uint8_t*)job_ring.buffer + head, src, size);
	job_ring.head = (head + size) % job_ring.size;
	job_ring.used += size;
	job_ring.free -= size;
}

static void job_ring_read(void* dest, const int32_t size)
{
	assert(size <= job_ring.used);
	const int32_t tail = job_ring.tail;
	memcpy(dest, (uint8_t*)job_ring.buffer + tail, size);
	job_ring.tail = (tail + size) % job_ring.size;
	job_ring.used -= size;
	job_ring.free += size;
}

static void job_thread_main(struct thread_context* ctx)
{
	while (atomic_load(&job_state) != JOBS_KILL)
	{
		if (atomic_load(&job_state) == JOBS_WORK)
		{
			while (atomic_fetch_or(&job_spinlock, SPIN_LOCK)) { ; }
			if (job_ring.used == 0)
			{
				atomic_store(&job_spinlock, SPIN_UNLOCK);
				continue;
			}
			struct job job;
			job_ring_read(&job, sizeof job);
			assert(job.arg_size <= JOB_ARG_CAP && "job arguments greater than 16KB!");
			if (job.arg_size > 0)
			{
				// spin until args fully written to the job_ring
				while(job_ring.used < job.arg_size) { ; }
				job_ring_read(ctx->args, job.arg_size);
			}
			atomic_store(&job_spinlock, SPIN_UNLOCK);
			ctx->scratch.size = 0;
			if (job.tag & INITIALZIER)
			{
				job.tag = 0x00;
				init_func func = job.fptr;
				func();
			}
			else
			{
				job_func func = job.fptr;
				func(ctx->args, ctx);
			}
		}
		else if (atomic_load(&job_state) == JOBS_SLEEP)// lock threads & do no work
		{
			/* pthread_barrier_wait(&sleep_barrier); */
			pthread_mutex_lock(&sleep_mtx);
			/* pthread_cond_wait(&sleep_cnd, &sleep_mtx); */
			pthread_mutex_unlock(&sleep_mtx);
		}
	}
}

void jobs_init(const int32_t num_threads, const int32_t min_job_ring_size)
{
	ring_map((struct ring_buffer*)&job_ring, min_job_ring_size, "jobring");
	job_ring.used = 0;
	job_ring.free = job_ring.size;
	job_vbuff = memalloc(PAGE_SIZE + (64 * KB + JOB_ARG_CAP) * num_threads);
	arena_t thread_setup_arena =
	{
		.allocation = job_vbuff.addr,
		.cap = PAGE_SIZE
	};
	arena_t thread_mem_arena =
	{
		.allocation = (uint8_t*)job_vbuff.addr + PAGE_SIZE,
		.cap = (64 * KB + JOB_ARG_CAP) * num_threads
	};
	job_threads = arena_calloc(&thread_setup_arena, num_threads * sizeof *job_threads);
	num_job_threads = num_threads;
	job_spinlock = SPIN_UNLOCK;
	job_state = JOBS_WORK;
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

void jobs_add_initializer(init_func initializer)
{
	struct job j = { .fptr = initializer };
	j.tag = INITIALZIER;
	job_ring_write(&j, sizeof j);
}


void __jobs_add_noarg(job_func job)
{
	struct job j = { .fptr = job };
	job_ring_write(&j, sizeof j);
}

void __jobs_add_arg(job_func job, struct buffer args)
{
	struct job j = { .fptr = job, .arg_size = args.size };
	job_ring_write(&j, sizeof j);
	job_ring_write(args.data, args.size);
}

static void wait_thread(void* args, struct thread_context* ctx)
{
	_Atomic(uint32_t)* wait_bitmask = (_Atomic(uint32_t)*)(*(uint64_t**)args);
	atomic_store(wait_bitmask, *wait_bitmask & ~ctx->mask);
	while (atomic_load(wait_bitmask)) { ; }
}

void jobs_finish()
{
	_Atomic(uint32_t) wait_bitmask = 0;
	for (int32_t i = 0; i < num_job_threads; ++i)
	{
		wait_bitmask |= 1 << i;
	}
	uint64_t** bits = (uint64_t**)&wait_bitmask;
	/* printf("main: %0lx vs %0lx\n", bits, &wait_bitmask); */
	for (int32_t i = 0; i < num_job_threads; ++i)
	{
		jobs_add(wait_thread, (struct buffer){.data = &bits, .size = sizeof bits });
	}
	if (job_state != JOBS_WORK)
	{
		jobs_wake();
	}
	while (atomic_load(&wait_bitmask)) { ; }
}


void jobs_sleep(void)
{
	if (job_state == JOBS_WORK)
	{
		pthread_mutex_lock(&sleep_mtx);
		job_state = JOBS_SLEEP;
	}
}

void jobs_wake(void)
{
	if (job_state == JOBS_SLEEP)
	{
		job_state = JOBS_WORK;
		pthread_mutex_unlock(&sleep_mtx);
	}
}

void jobs_kill(void)
{
	jobs_sleep();
	atomic_store(&job_state, JOBS_KILL);
	pthread_mutex_unlock(&sleep_mtx);
	for (int32_t i = 0; i < num_job_threads; ++i)
	{
		pthread_join(job_threads[i], NULL);
	}
	memfree(&job_vbuff);
	ring_unmap((ring_buffer*)&job_ring);
}
