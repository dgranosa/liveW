#ifndef _UTIL_H
#define _UTIL_H

#include <sys/time.h>

float getUnixTime()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (float)((int)tv.tv_sec % 1000000) + (float)(tv.tv_usec) / 1.0e6;
}

#endif
