#pragma once
DEFINE_bool(verbose, false, "");
DEFINE_int32(executor_num, 10, "number of executor threads");
DEFINE_int32(tnum, 1, "thread number");
DEFINE_int32(loop, 1, "loop times");

namespace quiet_flow{
namespace test{

#define PRINT(x)                     \
    if (FLAGS_verbose) {             \
        std::cout << x << std::endl; \
    }

#define ROUND for (int i = 0; i < FLAGS_round; i++)

#define SUICIDE()                                                 \
    do {                                                          \
        std::cerr << "raise SIGKILL to suicide ..." << std::endl; \
    } while (0 != raise(SIGKILL));                                \
    exit(-1);

#define CHECK_TRUE(x)                                   \
    if (x) {                                            \
    } else {                                            \
        std::cerr << "CHECK_TRUE failed." << std::endl; \
        SUICIDE()                                       \
    }

inline int fib(int n) {
    if (n <= 0)
        return 0;
    else if (n == 1)
        return 1;
    else
        return fib(n - 1) + fib(n - 2);
}

class NodeFib: public Node {
  private:
    int cnt;
  public:
    NodeFib(int i) {
        cnt = i;
        name_for_debug=(char)('A' + i - 1);
    }
    void run() {
        // std::cout << name_for_debug << "\n";
        fib(cnt);
        // std::cout << name_for_debug << " end\n";
    }
};

}}