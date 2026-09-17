#include "ticketlock.h"
#include <stdint.h>
#include <stdatomic.h>


void ticket_lock(struct ticketlock* lock)
{
	uint8_t num = atomic_fetch_add(&lock->next, 1);
	while(num != lock->owner) { ; }
}

void ticket_unlock(struct ticketlock* lock)
{
	atomic_fetch_add(&lock->owner, 1);
}
