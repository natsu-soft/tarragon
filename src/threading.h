#ifndef THREADING_H
#define THREADING_H

#include "threading_types.h"
#include <stdatomic.h>
#include <stdint.h>



void futex_lock(struct futex* ftx);
void futex_unlock(struct futex* ftx);

void barrier_wait(struct barrier* barrier);

void stdout_lock();
void stdout_unlock();

/* void cond_wait(struct conditional* conditional, struct futex* futex); */
/* void cond_signal(struct conditional* conditional); */
/* void cond_broadcast(struct conditional* conditional); */


#endif /* End THREADING_H */
