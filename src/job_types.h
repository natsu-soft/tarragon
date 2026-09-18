#ifndef JOB_TYPES_H
#define JOB_TYPES_H

#include <stdint.h>
#include "allocators/arena_types.h"


struct thread_context
{
	arena_t scratch;
	int32_t thread_num;
	uint32_t sync_epoch;
	uint32_t thread_mask;
};

typedef void (*job_func)(void* args, struct thread_context* ctx);

typedef void (*init_func)(void);

#endif /* End JOB_TYPES_H */
