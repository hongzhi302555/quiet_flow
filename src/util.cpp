#include "head/util.h"

namespace quiet_flow{
void bit_map_idx(const long int bit_map, size_t bit_size, std::vector<size_t>& idx_vec) {
    QuietFlowAssert(bit_size < 64);

    idx_vec.clear();
    idx_vec.reserve(bit_size);

    long int bit=1;
    for (size_t i=0; i<bit_size; i++) {
        if (bit & bit_map) {
            idx_vec.push_back(i);
        }
        bit <<= 1;
    }
}

void bit_map_set(long int& bit_map, size_t bit, size_t bit_size) {
    QuietFlowAssert(bit_size < 64);

    if (bit >= bit_size) {
        return;
    }
    bit_map |= ((long int)0x1 << bit);
}

bool bit_map_get(const long int bit_map, size_t bit, size_t bit_size) {
    QuietFlowAssert(bit_size < 64);

    if (bit >= bit_size) {
        return false;
    }

    return (bit_map & ((long int)0x1 << bit)) > 0;
}
}