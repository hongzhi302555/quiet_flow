#include "head/util.h"

namespace quiet_flow{
void bit_map_idx(long int bit_map, size_t bit_size, std::vector<size_t>& idx_vec) {
    idx_vec.reserve(bit_size);

    size_t bit=1;
    for (size_t i=0; i<bit_size; i++) {
        if (bit & bit_map) {
            idx_vec.push_back(i);
        }
        bit <<= 1;
    }
}

void bit_map_set(long int& bit_map, size_t bit, size_t bit_size) {
    if (bit >= bit_size) {
        return;
    }
    bit_map |= (0x1 << bit);
}

bool bit_map_get(const long int bit_map, size_t bit, size_t bit_size) {
    if (bit >= bit_size) {
        return false;
    }
    bit = 0x1 << bit;
    return (bit_map & bit) > 0;
}
}