#pragma once

#include <stdlib.h>
#include <sys/time.h>

namespace quiet_flow {

class TimeCost {
public:
    TimeCost() {
        gettimeofday(&_start_time, NULL);
    }

    long get_elapsed() const {
        struct timeval end_time;
        gettimeofday(&end_time, NULL);
        return (end_time.tv_sec - _start_time.tv_sec) * 1000 * 1000 
               + (end_time.tv_usec - _start_time.tv_usec);
    }

    void reset() {
        gettimeofday(&_start_time, NULL);
    }

    int64_t curr_us() {
        struct timeval end_time;
        gettimeofday(&end_time, NULL);
        return end_time.tv_sec * 1000 * 1000 + end_time.tv_usec;
    }

private:
    struct timeval _start_time;
};

} /* namespace quiet_flow */

