#include <atomic>
#include "head/util.h"

namespace quiet_flow{

class LocalErrorRate {
private:
    std::atomic<uint64_t> idx;
    std::atomic<uint64_t>* container;
    size_t bit_size;
    int high_bit;
private:
    LocalErrorRate(int bit_size_ = 128);
    size_t error_cnt();
public:
    void record_error();
    void record_success();
    double get() { return error_cnt() / 128.0;}
};
}