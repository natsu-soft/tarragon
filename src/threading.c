#include "threading.h"
#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include "atomics.h"
#include "sync_addr.h"

enum
{
	FTX_FREE = 0,
	FTX_LOCK,
	FTX_WAIT,
};

// READ: https://locklessinc.com/articles/barriers/
// READ: https://locklessinc.com/articles/phasers/

// based raymond chen explaining futex
// https://devblogs.microsoft.com/oldnewthing/20170601-00/?p=96265

void futex_lock(struct futex* ftx)
{
	int32_t expected;
	for (;;)
	{
		expected = FTX_FREE;
		if (atomic_compare_exchange_strong(&ftx->state, &expected, FTX_LOCK))
		{
			return;
		}
		// expected now either FTX_LOCK or FTX_WAIT.  Either case just WAIT it.
		else if (atomic_compare_exchange_strong(&ftx->state, &expected, FTX_WAIT))
		{
			expected = FTX_WAIT;
			sync_addr_wait(&ftx->state, &expected);
		}
	}
}

void futex_unlock(struct futex* ftx)
{
	if (atomic_exchange(&ftx->state, FTX_FREE) == FTX_WAIT)
	{
		sync_addr_wake_one(&ftx->state);
	}
}

void barrier_init(struct barrier* barrier, const int32_t count)
{
	store_ex(&barrier->count, count, seq_cst);
}

#include <stdio.h>

void barrier_wait(struct barrier* bar)
{
	/* futex_lock((struct futex*)bar); */
	int32_t value = fetch_sub_ex(&bar->count, 1, acq_rel);
	/* futex_unlock((struct futex*)bar); */
	if (value == 1)
	{
		return;
	}
	else if (value > 1)
	{
		while ((value = load_ex(&bar->count, acquire)) > 0) { ; }
		return;
	}
	else
	{
		assert(0 && "Hello, Fraud department? You're gonna wanna see this!");
	}
	/* int32_t value; */
	/* if ((value = fetch_sub_ex(&bar->count, 1, seq_cst)) == 1) */
	/* { */
	/* 	sync_addr_wake_all(&bar->count); */
	/* } */
	/* else if (value > 1) */
	/* { */
	/* 	while ((value = load_ex(&bar->count, acquire)) > 0) */
	/* 	{ */
	/* 		sync_addr_wait(&bar->count, &value); */
	/* 	} */
	/* } */
	/* else */
	/* { */
	/* 	assert(0 && "Hello, Fraud department? You're gonna wanna see this!"); */
	/* } */
}

// stack behavior not the best tbqh
void cond_wait(struct conditional* conditional, struct futex* futex)
{
	int32_t val = fetch_add_ex(&conditional->count, 1, acq_rel);
	futex_unlock(futex);
	while (val < load_ex(&conditional->count, acquire))
	{
		sync_addr_wait(&conditional->count, &val);
	}
	futex_lock(futex);
}

void cond_signal(struct conditional* conditional)
{
	if (load_ex(&conditional->count, acquire) > 0)
	{
		fetch_sub_ex(&conditional->count, 1, release);
		sync_addr_wake_all(&conditional->count);
	}
}

void cond_broadcast(struct conditional* conditional)
{
	if (swap_ex(&conditional->count, 0, acq_rel) > 0)
	{
		sync_addr_wake_all(&conditional->count);
	}
}



static struct futex stdout_futex = { 0 };
void global_ftx_lock()
{
	futex_lock(&stdout_futex);
}

void global_ftx_unlock()
{
	futex_unlock(&stdout_futex);
}

#include <pthread.h>
#include <stdio.h>

#define NUM_THREADS 256
#define BARRIER_NUM NUM_THREADS + 1

int barrier_wait_sancheck(void)
{
	pthread_t test_threads[NUM_THREADS] = {0};
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	struct barrier barrier;
	barrier_init(&barrier, NUM_THREADS + 1);
	for (int32_t i = 0; i < NUM_THREADS; ++i)
	{
		pthread_create(test_threads + i,  &attr, barrier_wait, &barrier);
	}
	barrier_wait(&barrier);
	for (int32_t i = 0; i < NUM_THREADS; ++i)
	{
		fflush(stdout);
		pthread_join(test_threads[i], NULL);
	}
	return 0;
}
#undef NUM_THREADS
