#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set>

#include "head/schedule.h"
#include "head/executor.h"
#include "head/node.h"

namespace quiet_flow{
namespace unit_test{

class TestInit: public ::testing::Test {
 protected:
  static void SetUpTestCase() {
  }
};

static void routine(Thread* thread_exec) {}

static ExectorItem* get_exactor_item() {
  ExectorItem::thread_idx_ = 0;
  auto thread_exec = new Thread(routine);
  auto item = new ExectorItem(thread_exec);
  return item;
}

class Node1: public quiet_flow::Node{ 
  public:
    void run(){};
};

TEST_F(TestInit, test_cas_ready_task) {
  ExectorItem* e = get_exactor_item();
  EXPECT_EQ(e->get_ready_task(), nullptr);

  Node1* t = new Node1();
  EXPECT_TRUE(e->cas_ready_task(nullptr, t));
  EXPECT_EQ(e->get_ready_task(), t);
  EXPECT_FALSE(e->cas_ready_task(nullptr, t));
  EXPECT_EQ(e->get_ready_task(), t);
  EXPECT_TRUE(e->cas_ready_task(t, nullptr));
  EXPECT_EQ(e->get_ready_task(), nullptr);
}

TEST_F(TestInit, test_atomic_set_bit_map_first_char) {
  std::vector<uint8_t> bit_map;
  bit_map.push_back(0);
  ExectorItem::atomic_set_bit_map(0, bit_map);
  EXPECT_EQ(bit_map[0], 0b10000000);
  ExectorItem::atomic_set_bit_map(1, bit_map);
  EXPECT_EQ(bit_map[0], 0b11000000);
  ExectorItem::atomic_set_bit_map(2, bit_map);
  EXPECT_EQ(bit_map[0], 0b11100000);
  ExectorItem::atomic_set_bit_map(3, bit_map);
  EXPECT_EQ(bit_map[0], 0b11110000);
  ExectorItem::atomic_set_bit_map(4, bit_map);
  EXPECT_EQ(bit_map[0], 0b11111000);
  ExectorItem::atomic_set_bit_map(5, bit_map);
  EXPECT_EQ(bit_map[0], 0b11111100);
  ExectorItem::atomic_set_bit_map(6, bit_map);
  EXPECT_EQ(bit_map[0], 0b11111110);
  ExectorItem::atomic_set_bit_map(7, bit_map);
  EXPECT_EQ(bit_map[0], 0b11111111);
 
  ExectorItem::thread_idx_ = 0;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map[0], 0b01111111); 
  ExectorItem::thread_idx_ = 1;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map[0], 0b00111111); 
  ExectorItem::thread_idx_ = 2;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map[0], 0b00011111); 
  ExectorItem::thread_idx_ = 3;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map[0], 0b00001111); 
  ExectorItem::thread_idx_ = 4;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map[0], 0b00000111); 
  ExectorItem::thread_idx_ = 5;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map[0], 0b00000011); 
  ExectorItem::thread_idx_ = 6;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map[0], 0b00000001); 
  ExectorItem::thread_idx_ = 7;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map[0], 0); 
}

TEST_F(TestInit, test_atomic_set_bit_map_more_char) {
  std::vector<uint8_t> bit_map;
  bit_map.push_back(0);
  bit_map.push_back(0);
  bit_map.push_back(0);
  bit_map.push_back(0);

  ExectorItem::atomic_set_bit_map(3, bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0, 0, 0}));
  ExectorItem::atomic_set_bit_map(10, bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0b00100000, 0, 0}));
  ExectorItem::atomic_set_bit_map(11, bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0b00110000, 0, 0}));
  ExectorItem::atomic_set_bit_map(14, bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0b00110010, 0, 0}));
  ExectorItem::atomic_set_bit_map(17, bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0b00110010, 0b01000000, 0}));
  ExectorItem::atomic_set_bit_map(22, bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0b00110010, 0b01000010, 0}));
  ExectorItem::atomic_set_bit_map(28, bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0b00110010, 0b01000010, 0b00001000}));

  ExectorItem::thread_idx_ = 28;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0b00110010, 0b01000010, 0}));
  ExectorItem::thread_idx_ = 22;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0b00110010, 0b01000000, 0}));
  ExectorItem::thread_idx_ = 17;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0b00110010, 0, 0}));
  ExectorItem::thread_idx_ = 14;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0b00110000, 0, 0}));
  ExectorItem::thread_idx_ = 11;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0b00100000, 0, 0}));
  ExectorItem::thread_idx_ = 10;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0, 0, 0}));
  ExectorItem::thread_idx_ = 9;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0b00010000, 0, 0, 0}));
  ExectorItem::thread_idx_ = 3;
  ExectorItem::atomic_clear_bit_map(bit_map);
  EXPECT_EQ(bit_map, std::vector<uint8_t>({0, 0, 0, 0}));
}

TEST_F(TestInit, test_get_free_exec) {
  std::vector<uint8_t>* bit_map;
  
  bit_map = new std::vector<uint8_t>({0b11111111});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), -1);
  bit_map = new std::vector<uint8_t>({0b01111111});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 0);
  bit_map = new std::vector<uint8_t>({0b01011111});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 0);
  bit_map = new std::vector<uint8_t>({0b10110111});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 1);
  bit_map = new std::vector<uint8_t>({0b11100101});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 3);
  bit_map = new std::vector<uint8_t>({0b11111010});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 5);
  bit_map = new std::vector<uint8_t>({0b11111110});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 7);


  bit_map = new std::vector<uint8_t>({0b11111111, 0b11111111});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), -1);
  bit_map = new std::vector<uint8_t>({0b11111111, 0b01111111});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 8);
  bit_map = new std::vector<uint8_t>({0b11111111, 0b01011111});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 8);
  bit_map = new std::vector<uint8_t>({0b11111111, 0b10110111});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 9);
  bit_map = new std::vector<uint8_t>({0b11111111, 0b11100101});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 11);
  bit_map = new std::vector<uint8_t>({0b11111111, 0b11100010});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 11);
  bit_map = new std::vector<uint8_t>({0b11111111, 0b11111110});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 15);
  bit_map = new std::vector<uint8_t>({0b11111111, 0b11111111, 0b11110110});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 20);
  bit_map = new std::vector<uint8_t>({0b11111111, 0b11111111, 0b11111111, 0b11110110});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 28);
}

TEST_F(TestInit, test_bit_map) {
  std::vector<uint8_t>* bit_map;
 
  bit_map = new std::vector<uint8_t>({0b11111111, 0b11111111, 0b11111111, 0b11110110});
  EXPECT_EQ(ExectorItem::get_free_exec(*bit_map), 28);

  ExectorItem::atomic_set_bit_map(28, *bit_map);
  EXPECT_EQ(*bit_map, std::vector<uint8_t>({0b11111111, 0b11111111, 0b11111111, 0b11111110}));

  ExectorItem::thread_idx_ = 28;
  ExectorItem::atomic_clear_bit_map(*bit_map);
  EXPECT_EQ(*bit_map, std::vector<uint8_t>({0b11111111, 0b11111111, 0b11111111, 0b11110110}));
}

}
} //set namespace
