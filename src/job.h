#ifndef JOB_H
#define JOB_H

#include <stdint.h>
#include "job_types.h"

#define JOB_H_GET_2ND(_1, _2, ...) _2
#define jobs_add(job, ...) JOB_H_GET_2ND(#__VA_ARGS__ __VA_OPT__(,) jobs_add_arg, jobs_add_noarg)(job __VA_OPT__(,) __VA_ARGS__)

// something to enqueue jobs

void jobs_init(const int32_t num_threads, const int32_t min_job_ring_size);
void jobs_add_initializer(init_func initializer);
void jobs_add_noarg(job_func job);
void jobs_add_arg(job_func job, void* args);
void jobs_sync(void);
void jobs_finish(void);


#endif /* End JOB_H */
