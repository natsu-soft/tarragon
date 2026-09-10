#ifndef SPINLOCK_TYPES_H
#define SPINLOCK_TYPES_H

#include <stdint.h>
#include <stdatomic.h>

// NOTE: https://h4x0r.org/futex/

typedef _Atomic(uint8_t) spinlock;

enum
{
	SPIN_UNLOCK = 0x00,
	SPIN_LOCK = 0x01,
};

#endif /* End SPINLOCK_TYPES_H */
