#include <atomic>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "head/util.h"
#include "head/error_rate.h"

namespace quiet_flow{

static int low_bit = (1<<6)-1;
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

LocalErrorRate::LocalErrorRate(int bit_size_) {
    idx.store(0, std::memory_order_relaxed);

    bit_size = align_size(bit_size_);
    int tmp = bit_size/64;
    container = (std::atomic<uint64_t>*)(calloc(tmp, sizeof(std::atomic<uint64_t>)));
    high_bit = tmp-1;
}

void LocalErrorRate::record_error() {
    uint64_t op = ((uint64_t)1) << (idx & low_bit);
    container[(idx>>6)&high_bit].fetch_or(op);
    idx.fetch_add(1, std::memory_order_relaxed);
}
void LocalErrorRate::record_success() {
    uint64_t op = ~(((uint64_t)1) << (idx & low_bit)); 
    container[(idx>>6)&high_bit].fetch_and(op);
    idx ++;
}

size_t LocalErrorRate::error_cnt() {
    size_t res = 0;
    for (size_t i=0; i<=high_bit; i++) {
        for (uint64_t c = container[i]; c>0; c >>= 8) {
            res += fast[c&(0xff)];
        }
    }
    return res;
}

}