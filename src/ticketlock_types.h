#ifndef TICKETLOCK_TYPES_H
#define TICKETLOCK_TYPES_H

#include <stdint.h>
#include <stdatomic.h>

typedef struct ticketlock
{
	_Atomic(uint8_t) owner;
	_Atomic(uint8_t) next;
} ticketlock;

#endif /* End TICKETLOCK_TYPES_H */
