/*
 * bench.c  – measure per-call latency of gettimeofday(2)
 *
 * Compile:  gcc -O2 -std=c11 -Wall -o bench bench.c
 *           # if you need -lrt on older glibc add it
 *
 * The program busy-loops for ~2 s (wall-clock, CLOCK_MONOTONIC),
 * timing each call to gettimeofday() with CLOCK_MONOTONIC and
 * printing:
 *
 *   num_reads:<N>
 *   average_read_time_usec:<µs>
 *   max_read_time_usec:<µs>
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

const double kBenchmarkLength = 5;

static inline uint64_t nsec_diff(const struct timespec *a,
                                 const struct timespec *b)
{
    return (a->tv_sec  - b->tv_sec)  * 1000000000ULL +
           (a->tv_nsec - b->tv_nsec);
}

int main(void)
{

    uint64_t total_nsec = 0;
    uint64_t max_nsec   = 0;
    uint64_t reads      = 0;

    struct timespec bench_start, now;
    clock_gettime(CLOCK_MONOTONIC, &bench_start);

    do {
        struct timespec t0, t1;
        struct timeval  tv;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        gettimeofday(&tv, NULL);
        clock_gettime(CLOCK_MONOTONIC, &t1);

        uint64_t dt = nsec_diff(&t1, &t0);
        total_nsec += dt;
        if (dt > max_nsec) max_nsec = dt;
        ++reads;

        clock_gettime(CLOCK_MONOTONIC, &now);
    } while (nsec_diff(&now, &bench_start) < (uint64_t)(kBenchmarkLength * 1e9));

    double avg_usec = (double)total_nsec / reads / 1000.0;
    double max_usec = (double)max_nsec   / 1000.0;

    printf("num_reads:%llu\n", (unsigned long long)reads);
    printf("average_read_time_usec:%f\n", avg_usec);
    printf("max_read_time_usec:%f\n", max_usec);
    return 0;
}
