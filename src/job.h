#ifndef JOB_H
#define JOB_H

#include <stdint.h>
#include "job_types.h"
#include "buffer_type.h"

// something to enqueue jobs

void job_threads_init(const int num_threads, const int32_t job_ring_size, const int32_t arg_ring_size);
void job_add(job_func job, struct buffer args);
void job_finish_all(void);
void job_threads_sleep(void);
void job_threads_wake(void);


#endif /* End JOB_H */
