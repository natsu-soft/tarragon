#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <time.h>
#include <sys/time.h>

#define MICROTIME(BODY) \
	{ \
		struct timeval micro_start; \
		struct timeval micro_stop;	\
		gettimeofday(&micro_start, NULL); \
		do {BODY} while(0); \
		gettimeofday(&micro_stop, NULL); \
		unsigned long long delta_ms = 1e6 * (micro_start.tv_sec - micro_stop.tv_sec) + micro_stop.tv_usec - micro_start.tv_usec; \
		printf("[MICROTIME] "#BODY " %06dus\n", delta_ms);	\
	}

#define LONGTIME(BODY) \
	{ \
		size_t t0 = time(NULL); \
		do { BODY } while(0); \
		size_t delta_s = time(NULL) - t0; \
		printf("[LONGTIME] "#BODY " %zu seconds\n", delta_s); \
	}

#endif /* End BENCHMARK_H */
