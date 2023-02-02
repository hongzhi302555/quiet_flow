#include <unistd.h>
#include <ucontext.h>
#include <atomic>

#include "head/error_rate.h"

namespace quiet_flow{

static unsigned int high_bit = (1<<6);
static unsigned int low_bit = (1<<6)-1;
static int fast[1<<8] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

void LocalErrorRate::record_error() {
    uint64_t op = ((uint64_t)1) << (idx & low_bit);
    if ((idx & high_bit) > 0) {
        container[1].fetch_or(op);
    } else {
        container[0].fetch_or(op);
    }
    idx.fetch_add(1, std::memory_order_relaxed);
}
void LocalErrorRate::record_success() {
    uint64_t op = ~(((uint64_t)1) << (idx & low_bit)); 
    if ((idx & high_bit) > 0) {
        container[1].fetch_and(op);
    } else {
        container[0].fetch_and(op);
    }
    idx ++;
}

size_t LocalErrorRate::error_cnt() {
    size_t res = 0;
    for (uint64_t c = container[0]; c>0; c >>= 8) {
        res += fast[c&(0xff)];
    }
    for (uint64_t c = container[1]; c>0; c >>= 8) {
        res += fast[c&(0xff)];
    }
    return res;
}

}