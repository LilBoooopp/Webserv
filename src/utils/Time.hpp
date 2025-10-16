#pragma once

#include <time.h>
#include <sys/time.h>

typedef unsigned long long u64;

inline u64 now_ms()
{
#ifdef CLOCK_MONOTONIC
	struct timespec	ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((u64)ts.tv_sec * 1000ULL + (u64)ts.tv_nsec / 1000000ULL);
#else
	struct timeval	tv;
	gettimeofday(&tv, 0);
	return ((u64)tv.tv_sec * 1000ULL + (u64)tv.tv_usec / 1000ULL);
#endif
}

inline u64 add_ms(u64 base, int ms)
{
	return (base + (u64)ms);
}
