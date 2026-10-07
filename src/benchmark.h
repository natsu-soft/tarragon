#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <time.h>
#include <sys/time.h>

#define MICROTIME(BODY, ...) \
	{ \
		struct timeval micro_start; \
		struct timeval micro_stop;	\
		gettimeofday(&micro_start, NULL); \
		do {BODY __VA_OPT__(,) __VA_ARGS__} while(0);	\
		gettimeofday(&micro_stop, NULL); \
		unsigned long long delta_ms = 1e6 * (micro_start.tv_sec - micro_stop.tv_sec) + micro_stop.tv_usec - micro_start.tv_usec; \
		printf("[MICROTIME] " #BODY #__VA_OPT__(,) #__VA_ARGS__ " %06dus\n", delta_ms); \
		/* fflush(stdout); */ \
	}

#define MICROMESSAGE(MSG, BODY)				\
	{ \
		struct timeval micro_start; \
		struct timeval micro_stop;	\
		gettimeofday(&micro_start, NULL); \
		do {BODY} while(0); \
		gettimeofday(&micro_stop, NULL); \
		unsigned long long delta_ms = 1e6 * (micro_start.tv_sec - micro_stop.tv_sec) + micro_stop.tv_usec - micro_start.tv_usec; \
		printf("[" MSG "] %06dus\n", delta_ms);	\
		fflush(stdout); \
	}

#define LONGTIME(BODY) \
	{ \
		size_t t0 = time(NULL); \
		do { BODY } while(0); \
		size_t delta_s = time(NULL) - t0; \
		printf("[LONGTIME] "#BODY " %zu seconds\n", delta_s); \
		fflush(stdout); \
	}

#define CLOCKMESSAGE(MSG, BODY, ...)				\
	{ \
		size_t t0 = time(NULL); \
		const clock_t c0 = clock(); \
		do { BODY __VA_OPT__(,) __VA_ARGS__ } while(0);	\
		const clock_t c1 = clock(); \
		const double elapsed = (double)(c1 - c0) / CLOCKS_PER_SEC;	\
		printf( MSG ": %f seconds\n", elapsed); \
		fflush(stdout); \
	}

#endif /* End BENCHMARK_H */
