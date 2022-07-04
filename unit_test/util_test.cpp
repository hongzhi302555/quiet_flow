#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set>

#include "head/util.h"

namespace quiet_flow{
namespace unit_test{

class TestInit: public ::testing::Test {
 protected:
  static void SetUpTestCase() {
  }
};

TEST_F(TestInit, test_set_bit) {
  long int bit_map = 0, bit_map_res = 0;
  bit_map_set(bit_map, 1, 0);
  EXPECT_EQ(bit_map_res, bit_map);

  bit_map = 0;
  bit_map_res = 0x1 << 1;
  bit_map_set(bit_map, 1, 2);
  EXPECT_TRUE(bit_map_get(bit_map, 1, 2));
  EXPECT_FALSE(bit_map_get(bit_map, 3, 2));
  EXPECT_FALSE(bit_map_get(bit_map, 0, 2));
  EXPECT_EQ(bit_map_res, bit_map);

  bit_map = 0;
  bit_map_res = 0x1;
  bit_map_set(bit_map, 0, 2);
  EXPECT_EQ(bit_map_res, bit_map);
  EXPECT_TRUE(bit_map_get(bit_map, 0, 2));
  EXPECT_FALSE(bit_map_get(bit_map, 3, 2));
  EXPECT_FALSE(bit_map_get(bit_map, 1, 2));

  size_t bit_ = 20;
  bit_map = 0;
  bit_map_res = 0x1; bit_map_res <<= bit_;
  bit_map_set(bit_map, bit_, 30);
  EXPECT_TRUE(bit_map_get(bit_map, bit_, 30));
  EXPECT_FALSE(bit_map_get(bit_map, 19, 30));
  EXPECT_FALSE(bit_map_get(bit_map, 21, 30));
  EXPECT_EQ(bit_map_res, bit_map);

  bit_ = 40;
  bit_map = 0;
  bit_map_res = 0x1; bit_map_res <<= bit_;
  bit_map_set(bit_map, bit_, 50);
  EXPECT_TRUE(bit_map_get(bit_map, bit_, 50));
  EXPECT_FALSE(bit_map_get(bit_map, 39, 50));
  EXPECT_FALSE(bit_map_get(bit_map, 41, 50));
  EXPECT_EQ(bit_map_res, bit_map);

  EXPECT_THROW(bit_map_get(bit_map, 4, 64), std::runtime_error);
  EXPECT_THROW(bit_map_set(bit_map, 4, 64), std::runtime_error);
  EXPECT_THROW(bit_map_set(bit_map, 4, 65), std::runtime_error);
}

TEST_F(TestInit, test_set) {
  long int bit_map = 0, bit_map_res = 0;
  std::vector<size_t> vec_s;

  bit_map_set(bit_map, 1, 0);
  EXPECT_EQ(bit_map_res, bit_map);

  bit_map = 0;
  bit_map_set(bit_map, 40, 63);
  bit_map_idx(bit_map, 63, vec_s);
  EXPECT_TRUE(bit_map_get(bit_map, 40, 63));
  EXPECT_FALSE(bit_map_get(bit_map, 39, 63));
  EXPECT_FALSE(bit_map_get(bit_map, 32, 63));
  EXPECT_FALSE(bit_map_get(bit_map, 41, 63));
  EXPECT_EQ(vec_s, std::vector<size_t>({40}));

  bit_map_set(bit_map, 32, 63);
  bit_map_idx(bit_map, 63, vec_s);
  EXPECT_TRUE(bit_map_get(bit_map, 40, 63));
  EXPECT_TRUE(bit_map_get(bit_map, 32, 63));
  EXPECT_FALSE(bit_map_get(bit_map, 41, 63));
  EXPECT_EQ(vec_s, std::vector<size_t>({32, 40}));

  bit_map_set(bit_map, 18, 63);
  bit_map_idx(bit_map, 63, vec_s);
  EXPECT_TRUE(bit_map_get(bit_map, 40, 63));
  EXPECT_TRUE(bit_map_get(bit_map, 32, 63));
  EXPECT_TRUE(bit_map_get(bit_map, 18, 63));
  EXPECT_FALSE(bit_map_get(bit_map, 41, 63));
  EXPECT_EQ(vec_s, std::vector<size_t>({18, 32, 40}));

  bit_map_set(bit_map, 62, 63);
  bit_map_idx(bit_map, 63, vec_s);
  EXPECT_TRUE(bit_map_get(bit_map, 40, 63));
  EXPECT_TRUE(bit_map_get(bit_map, 32, 63));
  EXPECT_TRUE(bit_map_get(bit_map, 18, 63));
  EXPECT_TRUE(bit_map_get(bit_map, 62, 63));
  EXPECT_FALSE(bit_map_get(bit_map, 41, 63));
  EXPECT_EQ(vec_s, std::vector<size_t>({18, 32, 40, 62}));

  bit_map_set(bit_map, 0, 63);
  bit_map_idx(bit_map, 63, vec_s);
  EXPECT_TRUE(bit_map_get(bit_map, 40, 63));
  EXPECT_TRUE(bit_map_get(bit_map, 32, 63));
  EXPECT_TRUE(bit_map_get(bit_map, 18, 63));
  EXPECT_TRUE(bit_map_get(bit_map, 62, 63));
  EXPECT_FALSE(bit_map_get(bit_map, 41, 63));
  EXPECT_FALSE(bit_map_get(bit_map, 63, 63));
  EXPECT_EQ(vec_s, std::vector<size_t>({0, 18, 32, 40, 62}));

  EXPECT_THROW(bit_map_idx(bit_map, 64, vec_s), std::runtime_error);
}
}
} //set namespace