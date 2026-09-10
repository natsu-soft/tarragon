#ifndef JOB_TYPES_H
#define JOB_TYPES_H

#include <stdint.h>
#include "buffer_type.h"
#include "allocators/arena_types.h"

enum job_state
{
	JOBS_SLEEP = 0x00,
	JOBS_WORK,
	JOBS_KILL,
};

struct thread_context
{
	void* args;
	arena_t scratch;
	int32_t num;
	uint32_t mask;
};

typedef void (*job_func)(void* args, struct thread_context* ctx);

#endif /* End JOB_TYPES_H */
