#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "spinlock_types.h"

void spin_lock(spinlock* spinlock);
void spin_unlock(spinlock* spinlock);


#endif /* End SPINLOCK_H */
