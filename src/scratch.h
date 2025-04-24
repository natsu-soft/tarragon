#ifndef SCRATCH_H
#define SCRATCH_H

#include <stddef.h>

// YO THIS IS FUCKED
/* #define SCRATCH(PTR_DEC, BYTES) PTR_DEC __attribute__((cleanup(scratch_free))) = scratch_alloc(BYTES) */
/* #define SCRATCH(PTR_DEC) __attribute__((cleanup(scratch_free)) PTR_DEC */
// YEAH, this definitely won't shoot me in the foot later
#define SCRATCH __attribute__((cleanup(scratch_free)))
/* #define SCRATCH(PTR_DEC) PTR_DEC __attribute__((cleanup(scratch_free))) */
/* #define SCRATCH(bytes) __attribute__((cleanup(scratch_free))) = scratch_alloc(bytes) */
// I HOPE this works out since im not multitreading
// (single-threaded scratch mimics a stack IDEALLY)

void* scratch_alloc(const size_t bytes);

void scratch_free(void* ptr);

#endif /* End SCRATCH_H */
