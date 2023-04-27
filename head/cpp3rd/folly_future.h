#pragma once

#ifdef QUIET_FLOW_FAKE_FOLLY
#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include "head/locks/thread.h"

namespace folly {
template <class T>
class InnerFuture {
 public:
  using CallBackFunc = std::function<void (T&)>;
  std::mutex mutex_;
  bool finish = false;
  quiet_flow::locks::Semaphore sema;
  std::vector<CallBackFunc> call_backs;
  std::vector<CallBackFunc> error_call_backs;
  std::function<void()> finish_call_back;
  T v;
};
template <class T>
class Future {
 public:
  using CallBackFunc = std::function<void (T&)>;
  std::shared_ptr<InnerFuture<T>> f;
 public:
  Future<T>&& then(CallBackFunc&& func) {
    f->call_backs.push_back(func);
    return std::move(*this);
  }
  Future<T>&& onError(CallBackFunc&& func) {
    f->error_call_backs.push_back(func);
    return std::move(*this);
  }
  Future<T>&& ensure(std::function<void()>&& func) {
    f->mutex_.lock();
    if (f->finish) {
      func();
    } else {
      f->finish_call_back = func;
    }
    f->mutex_.unlock();
    return std::move(*this);
  }
  T get() {
    f->sema.wait();
    return f->v;
  }
};
template <class T>
class Promise {
 private:
  Future<T> f;
 public:
  Promise() {
    f.f = std::make_shared<InnerFuture<T>>();
    f.f->finish_call_back = [](){};
  }
  Future<T> getFuture() {
    return f;
  }
  void setValue(T v) {
    try {
      for (auto& c: f.f->call_backs) {
        c(v);
      }
    } catch(...) {
      for (auto& c: f.f->error_call_backs) {
        c(v);
      }
    }
    f.f->mutex_.lock();
    f.f->finish = true;
    f.f->finish_call_back();
    f.f->v = v;
    f.f->sema.signal();
    f.f->mutex_.unlock();
  }

};
}
#else
#include <folly/futures/Future.h>
#endif 

