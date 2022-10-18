#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set> 
#include <random>
#include <algorithm>
#include <thread>
#include <iterator>

#include "head/util.h"
#include "head/queue/free_lock.h"
#include "head/queue/lock.h"

namespace quiet_flow{
namespace queue{
namespace free_lock{
// namespace lock{

namespace unit_test{

class LimitQueueTest: public LimitQueue {
  public:
    LimitQueueTest(uint64_t size):LimitQueue(size) {
      size += (size & 7) ? 8 : 0;
      size &= ~(uint64_t)7;
      capacity = size;
      #ifdef QUIET_FLOW_DEBUG
      vec.reserve(capacity);
      #else
      vec = (void**)(calloc(capacity, sizeof(void*)));
      #endif
      capacity_bitmap = capacity - 1;
    }
    virtual ~LimitQueueTest(){};
};

class TestFreeLockQueueConEn: public ::testing::Test {
  public:
    int size = 6;
		std::atomic<int> i;
    LimitQueueTest* queue_ptr;
    std::vector<bool> res;
    std::vector<std::thread*> threads;
  public:
    void wait_group() {
      for (auto t: threads) {
        t->join();
      }
    }

  protected:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    static void func(TestFreeLockQueueConEn* t, uint64_t v) {
      while (t->i.load(std::memory_order_acquire) == 0);
      t->res[v] = t->queue_ptr->try_enqueue(new int(v));
    }
    void SetUp() {
      i = 0;
      queue_ptr = new LimitQueueTest(size);
      res.resize(10);
      for (uint64_t i=0; i<10; i++) {
        threads.push_back(new std::thread(&func, this, i));
      }
    }
    void TearDown() {
      i = 0;
      delete queue_ptr;
    }
};

void f(TestFreeLockQueueConEn* t, int v) {
  LimitQueueTest& queue = *t->queue_ptr;
  uint64_t fill_size = 8-v;
  for (auto i=0; i<fill_size ; i++) {
    queue.try_enqueue(new int(i+100));
  }
  EXPECT_EQ(queue.size_approx(), fill_size);
  EXPECT_EQ(queue.tail, fill_size);
  EXPECT_EQ(queue.head, 0);
  // EXPECT_FALSE(queue.is_full(queue.head, queue.tail));

  t->i.store(1, std::memory_order_relaxed);
  t->wait_group();
  // EXPECT_TRUE(queue.is_full(queue.head, queue.tail));

  std::unordered_set<int> s;
  void* m;
  for (auto i=0; i<fill_size; i++) {
    queue.try_dequeue(&m);
  }
  for (auto i=fill_size; i<8; i++) {
    queue.try_dequeue(&m);
    s.insert(*(int*)m);
  }

  queue.try_dequeue(&m);
  s.insert(*(int*)m);

  // EXPECT_TRUE(queue.is_empty(queue.head, queue.tail));

  for (int i=0; i<t->res.size(); i++) {
    if (t->res[i]) {
      EXPECT_TRUE(s.find(i) != s.end());
    }
  }
}

TEST_F(TestFreeLockQueueConEn, test_con_en1) {
  f(this, 1);
}

TEST_F(TestFreeLockQueueConEn, test_con_en2) {
  f(this, 2);
}

TEST_F(TestFreeLockQueueConEn, test_con_en3) {
  f(this, 3);
}

TEST_F(TestFreeLockQueueConEn, test_con_en4) {
  f(this, 4);
}

TEST_F(TestFreeLockQueueConEn, test_con_en5) {
  f(this, 5);
}

TEST_F(TestFreeLockQueueConEn, test_con_en6) {
  f(this, 6);
}

TEST_F(TestFreeLockQueueConEn, test_con_en7) {
  f(this, 7);
}

TEST_F(TestFreeLockQueueConEn, test_con_en8) {
  f(this, 8);
}

class TestFreeLockQueueConDe: public ::testing::Test {
  public:
    int size = 6;
		std::atomic<int> i;
    LimitQueueTest* queue_ptr;
    std::vector<bool> res;
    std::vector<int> v_res;
    std::vector<std::thread*> threads;
  public:
    void wait_group() {
      for (auto t: threads) {
        t->join();
      }
    }

  protected:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    static void func(TestFreeLockQueueConDe* t, uint64_t v) {
      while (t->i.load(std::memory_order_acquire) == 0);
      void* m;
      t->res[v] = t->queue_ptr->try_dequeue(&m);
      if (t->res[v]) {
        t->v_res[v] = *(int*)m;
      } else {
        t->v_res[v] = -10000;
      }
    }
    void SetUp() {
      i = 0;
      queue_ptr = new LimitQueueTest(size);
      res.resize(10);
      v_res.resize(10);
      for (uint64_t i=0; i<10; i++) {
        threads.push_back(new std::thread(&func, this, i));
      }
    }
    void TearDown() {
      i = 0;
      delete queue_ptr;
    }
};

void f(TestFreeLockQueueConDe* t, int fill_size) {
  LimitQueueTest& queue = *t->queue_ptr;
  for (auto i=0; i<fill_size ; i++) {
    queue.try_enqueue(new int(i+100));
  }
  EXPECT_EQ(queue.size_approx(), fill_size);
  EXPECT_EQ(queue.tail, fill_size);
  EXPECT_EQ(queue.head, 0);
  // EXPECT_FALSE(queue.is_empty(queue.head, queue.tail));

  t->i.store(1, std::memory_order_relaxed);
  t->wait_group();
  EXPECT_EQ(queue.head, fill_size);
  // EXPECT_TRUE(queue.is_empty(queue.head, queue.tail));

  for (int i=0; i<t->res.size(); i++) {
    if (t->res[i]) {
      EXPECT_TRUE(t->v_res[i] != -10000);
    }
  }
}

TEST_F(TestFreeLockQueueConDe, test_con_en1) {
  f(this, 1);
}

TEST_F(TestFreeLockQueueConDe, test_con_en2) {
  f(this, 2);
}

TEST_F(TestFreeLockQueueConDe, test_con_en3) {
  f(this, 3);
}

TEST_F(TestFreeLockQueueConDe, test_con_en4) {
  f(this, 4);
}

TEST_F(TestFreeLockQueueConDe, test_con_en5) {
  f(this, 5);
}

TEST_F(TestFreeLockQueueConDe, test_con_en6) {
  f(this, 6);
}

TEST_F(TestFreeLockQueueConDe, test_con_en7) {
  f(this, 7);
}

TEST_F(TestFreeLockQueueConDe, test_con_en8) {
  f(this, 8);
}

class TestFreeLockQueueConEnDe: public ::testing::Test {
  public:
    int size = 6;
		std::atomic<int> i;
    LimitQueueTest* queue_ptr;
    std::vector<int> en_res;
    std::vector<int> de_res;
    std::vector<int> v_res;
    std::vector<std::thread*> threads;
  public:
    void wait_group() {
      for (auto t: threads) {
        t->join();
      }
    }
    static void func_de(TestFreeLockQueueConEnDe* t, uint64_t v) {
      while (t->i.load(std::memory_order_acquire) == 0);
      void* m;
      t->de_res[v] = t->queue_ptr->try_dequeue(&m);
      if (t->de_res[v]) {
        t->v_res[v] = *(int*)m;
      } else {
        t->v_res[v] = -10000;
      }
    }
    static void func_en(TestFreeLockQueueConEnDe* t, uint64_t v) {
      while (t->i == 0);
      t->en_res[v] = t->queue_ptr->try_enqueue(new int(v));
    }

  protected:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {
      i = 0;
      queue_ptr = new LimitQueueTest(size);
    }
    void TearDown() {
      i = 0;
      delete queue_ptr;
    }
};

void f(TestFreeLockQueueConEnDe* t, int en, int de) {
  LimitQueueTest& queue = *t->queue_ptr;
  t->en_res.resize(en);
  t->v_res.resize(de);
  t->de_res.resize(de);
  t->threads.reserve(100);
  for (uint64_t i=0; i<de; i++) {
    t->threads.push_back(new std::thread(&TestFreeLockQueueConEnDe::func_de, t, i));
  }
  for (uint64_t i=0; i<en; i++) {
    t->threads.push_back(new std::thread(&TestFreeLockQueueConEnDe::func_en, t, i));
  }
  
  t->i.store(1, std::memory_order_relaxed);
  t->wait_group();

  int de_t = 0, en_t = 0;
  std::unordered_set<int> en_set;
  for (int i=0; i<t->en_res.size(); i++) {
    if (t->en_res[i]) {
      en_set.insert(i);
      en_t ++;
    }
  }
  EXPECT_EQ(queue.tail, en_t);
  if (en <= 8) {
    EXPECT_EQ(en, en_t);
  }

  std::unordered_set<int> de_set;
  for (int i=0; i<t->de_res.size(); i++) {
    if (t->de_res[i]) {
      EXPECT_TRUE(t->v_res[i] != -10000);
      de_set.insert(t->v_res[i]);
      de_t ++;
    }
  }

  EXPECT_EQ(queue.head, de_t);
  EXPECT_TRUE(de_t <= en_t);
  EXPECT_EQ(queue.size_approx(), (en_t - de_t));

  std::unordered_set<int> left_set;
  void* m;
  while (queue.try_dequeue(&m)) {
    left_set.insert(*(int*)m);
  }
  EXPECT_EQ(en_set.size(), (de_set.size() + left_set.size()));

  for (auto l_iter=left_set.begin(); l_iter != left_set.end(); l_iter++) {
    de_set.insert(*l_iter);
  }
  EXPECT_EQ(en_set, de_set);
}

TEST_F(TestFreeLockQueueConEnDe, test_con_ende1) {
  f(this, 3, 5);
}

TEST_F(TestFreeLockQueueConEnDe, test_con_ende2) {
  f(this, 4, 5);
}

TEST_F(TestFreeLockQueueConEnDe, test_con_ende3) {
  f(this, 4, 9);
}

TEST_F(TestFreeLockQueueConEnDe, test_con_ende4) {
  f(this, 7, 10);
}

TEST_F(TestFreeLockQueueConEnDe, test_con_ende5) {
  f(this, 10, 10);
}

}
}
}
} //set namespace