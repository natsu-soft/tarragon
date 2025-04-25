#ifndef SCRATCH_H
#define SCRATCH_H

#include <stddef.h>

// YEAH, this definitely won't shoot me in the foot later
// NOTE: gcc bug: calls immeadiately
// NOTE: clang bug: NEVER calls >:^(
#define SCRATCH __attribute__((cleanup(scratch_free)))

void* scratch_alloc(const size_t bytes);

// you don't HAVE to call this, but you can.
void scratch_free(void* ptr);

void scratch_free_all();

void scratch_audit();

#endif /* End SCRATCH_H */
