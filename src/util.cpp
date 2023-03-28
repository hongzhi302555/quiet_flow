#include "head/util.h"
#include <iostream>

namespace quiet_flow{
void bit_map_idx(const long int bit_map, uint64_t bit_size, std::vector<uint64_t>& idx_vec) {
    QuietFlowAssert(bit_size < 64);

    idx_vec.clear();
    idx_vec.reserve(bit_size);

    long int bit=1;
    for (uint64_t i=0; i<bit_size; i++) {
        if (bit & bit_map) {
            idx_vec.push_back(i);
        }
        bit <<= 1;
    }
}

void bit_map_set(long int& bit_map, uint64_t bit, uint64_t bit_size) {
    QuietFlowAssert(bit_size < 64);

    if (bit >= bit_size) {
        return;
    }
    bit_map |= ((long int)0x1 << bit);
}

bool bit_map_get(const long int bit_map, uint64_t bit, uint64_t bit_size) {
    QuietFlowAssert(bit_size < 64);

    if (bit >= bit_size) {
        return false;
    }

    return (bit_map & ((long int)0x1 << bit)) > 0;
}

uint64_t align_size(uint64_t size) {
    auto first_bit = std::__lg(size);
    return ((1<<first_bit) == size) ? size : (1<<(first_bit+1));
}

StdOut::~StdOut() {
    oss << "\n";
    std::cout << oss.str() << std::endl;
}

}