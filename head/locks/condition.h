#pragma once

#include <atomic>
#include <unistd.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace quiet_flow{

class Signal {
  private:
    enum class Triggered{
      LT,   // 水平触发
      ET,   // 边沿触发
    };
    class SignalItem {
      public:
        SignalItem();
        ~SignalItem();
      public:
        Node* node;
        Graph* g;
    };
  private:
    std::atomic<int> pending_cnt_;
    ConcurrentQueue<SignalItem*> pending_node_;
    Triggered triger_;
  private:
    SignalItem* append();
    SignalItem* pop();
  public:
    Signal(Triggered triger=Triggered::ET): pending_cnt_(0), triger_(triger) {};
    ~Signal() = default;
    void wait(long timeout);
    void notify();
};

}