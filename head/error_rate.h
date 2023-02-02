#include <unistd.h>
#include <ucontext.h>
#include <atomic>

namespace quiet_flow{
class LocalErrorRate {
private:
    std::atomic<uint64_t> idx;
    std::atomic<uint64_t> container[2];
    LocalErrorRate() {
        idx.store(0, std::memory_order_relaxed);
        container[0].store(0, std::memory_order_relaxed);
        container[1].store(0, std::memory_order_relaxed);
    }
    size_t error_cnt();
public:
    void record_error();
    void record_success();
    double get() { return error_cnt() / 128.0;}
};
}