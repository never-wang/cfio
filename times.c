#include <time.h>
#include <sys/time.h>

#include "times.h"

double cur_time()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec * 1000.0 + time.tv_usec / 1000.0;
}

