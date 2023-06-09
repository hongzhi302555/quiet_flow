#pragma once

#include <sstream>
#include <exception>
#include <stdexcept>
#include <vector>

#ifdef UNIT_TEST
  #define private public
  #define protected public
#endif

namespace quiet_flow{

using uint64_t = unsigned long;
using int64_t = signed long;
using int32_t = signed;

enum class RunningStatus{
    Initing,
    Ready,
    Running,
    Yield,
    Finish,
    Fail,
    Recoverable,
    Destroy,
};

class LogMessageFatal {
 public:
  LogMessageFatal(const char* file, int line) {
    log_stream_ << file << ":" << line << ": ";
  }
  std::ostringstream& stream() {
    return log_stream_;
  }
  ~LogMessageFatal() noexcept(false) {
    throw std::runtime_error(log_stream_.str());
  }

 private:
  std::ostringstream log_stream_;
};

#define QuietFlowAssert(x)                                        \
  if (!(x)) {                                                     \
    LogMessageFatal(__FILE__, __LINE__).stream()                  \
      << "QuietFlow failed: " #x << ": ";}

void bit_map_idx(const long int bit_map, uint64_t bit_size, std::vector<uint64_t>& idx_vec);
void bit_map_set(long int& bit_map, uint64_t bit, uint64_t bit_size);
bool bit_map_get(const long int bit_map, uint64_t bit, uint64_t bit_size);

uint64_t align_size(uint64_t size);

class StdOut {
  private:
    std::ostringstream oss;
  public:
    StdOut() = default;
    ~StdOut();
  public:
    template<class T>
    StdOut& operator<<(T &v){
        oss << " " << v;
        return *this;
    }
    StdOut& operator<<(unsigned long i){
        oss << i;
        return *this;
    }
    
};

}