#ifndef THREADING_H
#define THREADING_H

#include "threading_types.h"
#include <stdatomic.h>
#include <stdint.h>



void futex_lock(struct futex* ftx);
void futex_unlock(struct futex* ftx);

// zero struct initialization is atomic, so futex doesnt need an initializer
// BUT BARRIERS DO LMAO
void barrier_init(struct barrier* barrier, const int32_t count);
void barrier_wait(struct barrier* barrier);

void global_ftx_lock();
void global_ftx_unlock();

#define ftx_printf(A, ...) do { global_ftx_lock(); printf(A __VA_OPT__(,) __VA_ARGS__); global_ftx_unlock(); } while(0);

void cond_wait(struct conditional* conditional, struct futex* futex);
void cond_signal(struct conditional* conditional);
void cond_broadcast(struct conditional* conditional);


#endif /* End THREADING_H */
