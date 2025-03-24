#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <sys/time.h>

#define NANOTIME(FUNC_CALL) \
	{ \
		struct timeval micro_start, micro_stop;	\
		gettimeofday(&micro_start, NULL); \
		do {FUNC_CALL} while(0);		  \
		gettimeofday(&micro_stop, NULL); \
		printf("[NANOTIME] "#FUNC_CALL": %dus\n", micro_stop.tv_usec - micro_start.tv_usec); \
	}

#endif /* End BENCHMARK_H */
