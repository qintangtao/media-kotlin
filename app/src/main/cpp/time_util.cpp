//
// Created by Administrator on 2022/4/15.
//

#include "time_util.h"
#include <pthread.h>

int64_t gettime_relative()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}
