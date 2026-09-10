#ifndef JOB_H
#define JOB_H

#include <stdint.h>
#include "job_types.h"
#include "buffer_type.h"

#define __GET_2(_1, _2, ...) _2
#define jobs_add(job, ...) __GET_2(#__VA_ARGS__ __VA_OPT__(,) __jobs_add_arg, __jobs_add_noarg)(job __VA_OPT__(,) __VA_ARGS__)

// something to enqueue jobs

void jobs_init(const int32_t num_threads, const int32_t min_job_ring_size);
void jobs_add_initializer(init_func initializer);
void jobs_add_noarg(job_func job);
void jobs_add_arg(job_func job, struct buffer args);
void jobs_finish(void);
void jobs_sleep(void);
void jobs_wake(void);
void jobs_kill(void);


#endif /* End JOB_H */
