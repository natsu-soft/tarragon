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
	if (fetch_sub_ex(&bar->count, 1, seq_cst) <= 1)
	{
		sync_addr_wake_all(&bar->count);
	}
	else
	{
		int32_t current;
		while ((current = load_ex(&bar->count, acquire)) > 0)
		{
			sync_addr_wait(&bar->count, &current);
		}
	}
}

static struct futex stdout_futex = { 0 };
void stdout_lock()
{
	futex_lock(&stdout_futex);
}

void stdout_unlock()
{
	futex_unlock(&stdout_futex);
}

#include <pthread.h>
#include <stdio.h>

#define NUM_THREADS 256
#define BARRIER_NUM NUM_THREADS + 1

int barrier_wait_test(int argc, char** argv)
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
