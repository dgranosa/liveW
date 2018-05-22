#ifndef _UTIL_H
#define _UTIL_H

#include <sys/time.h>

struct timeval start;

float getUnixTime()
{
	struct timeval tv;
    gettimeofday(&tv, 0);
    return (float)(tv.tv_sec - start.tv_sec) + (float)(tv.tv_usec) / 1.0e6;
}

#endif
