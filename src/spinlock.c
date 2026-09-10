#include "spinlock.h"
#include <stdatomic.h>


void spin_lock(spinlock* spinlock)
{
	while(atomic_fetch_or(spinlock, SPIN_LOCK));
}

void spin_unlock(spinlock* spinlock)
{
	atomic_store(spinlock, SPIN_UNLOCK);
}
