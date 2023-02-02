#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set>

#include "head/util.h"
#include "head/error_rate.h"

namespace quiet_flow{
namespace unit_test{

class TestErrorRate: public ::testing::Test {
 protected:
  static void SetUpTestCase() {
  }
};

TEST_F(TestErrorRate, test_record_low_1) {
  auto* r = new LocalErrorRate();
  auto& c = r->container[0];

  int cnt = 1;
  r->record_error();
  EXPECT_EQ(c, 0b1);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b11);
  EXPECT_EQ(r->error_cnt(), cnt);
  

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b111);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b1111);
  EXPECT_EQ(r->error_cnt(), cnt);


  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b11111);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b111111);
  EXPECT_EQ(r->error_cnt(), cnt);

  r->record_success();
  EXPECT_EQ(c, 0b111111);
  r->record_success();
  EXPECT_EQ(c, 0b111111);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b100111111);
  EXPECT_EQ(r->error_cnt(), cnt);
}

TEST_F(TestErrorRate, test_record_high_1) {
  auto* r = new LocalErrorRate();

  r->idx.store(64);
  auto& c = r->container[1];

  int cnt = 1;
  r->record_error();
  EXPECT_EQ(c, 0b1);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b11);
  EXPECT_EQ(r->error_cnt(), cnt);
  

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b111);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b1111);
  EXPECT_EQ(r->error_cnt(), cnt);


  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b11111);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b111111);
  EXPECT_EQ(r->error_cnt(), cnt);

  r->record_success();
  EXPECT_EQ(c, 0b111111);
  r->record_success();
  EXPECT_EQ(c, 0b111111);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b100111111);
  EXPECT_EQ(r->error_cnt(), cnt);
}

TEST_F(TestErrorRate, test_record_hl_1) {
  auto* r = new LocalErrorRate();

  r->idx.store(62);

  int cnt = 1;
  r->record_error();
  EXPECT_EQ(r->container[0], 0x4000000000000000);
  EXPECT_EQ(r->error_cnt(), cnt);
  cnt ++;
  r->record_error();
  EXPECT_EQ(r->container[0], 0xc000000000000000);
  EXPECT_EQ(r->error_cnt(), cnt);

  auto& c = r->container[1];
  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b1);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b11);
  EXPECT_EQ(r->error_cnt(), cnt);
  

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b111);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b1111);
  EXPECT_EQ(r->error_cnt(), cnt);


  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b11111);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b111111);
  EXPECT_EQ(r->error_cnt(), cnt);

  r->record_success();
  EXPECT_EQ(c, 0b111111);
  r->record_success();
  EXPECT_EQ(c, 0b111111);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b100111111);
  EXPECT_EQ(r->error_cnt(), cnt);
}

TEST_F(TestErrorRate, test_record_hl_2) {
  auto* r = new LocalErrorRate();

  r->idx.store(190);

  int cnt = 1;
  r->record_error();
  EXPECT_EQ(r->container[0], 0x4000000000000000);
  EXPECT_EQ(r->error_cnt(), cnt);
  cnt ++;
  r->record_error();
  EXPECT_EQ(r->container[0], 0xc000000000000000);
  EXPECT_EQ(r->error_cnt(), cnt);

  auto& c = r->container[1];
  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b1);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b11);
  EXPECT_EQ(r->error_cnt(), cnt);
  

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b111);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b1111);
  EXPECT_EQ(r->error_cnt(), cnt);


  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b11111);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b111111);
  EXPECT_EQ(r->error_cnt(), cnt);

  r->record_success();
  EXPECT_EQ(c, 0b111111);
  r->record_success();
  EXPECT_EQ(c, 0b111111);
  EXPECT_EQ(r->error_cnt(), cnt);

  cnt ++;
  r->record_error();
  EXPECT_EQ(c, 0b100111111);
  EXPECT_EQ(r->error_cnt(), cnt);
}

TEST_F(TestErrorRate, test_error_cnt_1) {
  auto* r = new LocalErrorRate();

  r->container[0] = 0xc000000000000000;
  EXPECT_EQ(r->error_cnt(), 2);
  r->container[0] = 0xc000000c00000000;
  EXPECT_EQ(r->error_cnt(), 4);
  r->container[0] = 0xc000000c00000c00;
  EXPECT_EQ(r->error_cnt(), 6);
  r->container[0] = 0xc000000c00000c01;
  EXPECT_EQ(r->error_cnt(), 7);
  r->container[0] = 0xc000000c00000c03;
  EXPECT_EQ(r->error_cnt(), 8);
  r->container[0] = 0xc000000c00000c04;
  EXPECT_EQ(r->error_cnt(), 7);
  r->container[0] = 0xc000000c00040c04;
  EXPECT_EQ(r->error_cnt(), 8);
  r->container[0] = 0xc000000c01040c04;
  EXPECT_EQ(r->error_cnt(), 9);
  r->container[0] = 0xc007000c01040c04;
  EXPECT_EQ(r->error_cnt(), 12);
}

TEST_F(TestErrorRate, test_error_cnt_2) {
  auto* r = new LocalErrorRate();

  r->container[1] = 0xc000000000000000;
  EXPECT_EQ(r->error_cnt(), 2);
  r->container[1] = 0xc000000c00000000;
  EXPECT_EQ(r->error_cnt(), 4);
  r->container[1] = 0xc000000c00000c00;
  EXPECT_EQ(r->error_cnt(), 6);
  r->container[1] = 0xc000000c00000c01;
  EXPECT_EQ(r->error_cnt(), 7);
  r->container[1] = 0xc000000c00000c03;
  EXPECT_EQ(r->error_cnt(), 8);
  r->container[1] = 0xc000000c00000c04;
  EXPECT_EQ(r->error_cnt(), 7);
  r->container[1] = 0xc000000c00040c04;
  EXPECT_EQ(r->error_cnt(), 8);
  r->container[1] = 0xc000000c01040c04;
  EXPECT_EQ(r->error_cnt(), 9);
  r->container[1] = 0xc007000c01040c04;
  EXPECT_EQ(r->error_cnt(), 12);
}

TEST_F(TestErrorRate, test_error_cnt_3) {
  auto* r = new LocalErrorRate();

  r->container[0] = 0xc000000c00300c03;
  r->container[1] = 0xc000000000000000;
  EXPECT_EQ(r->error_cnt(), 12);
  r->container[1] = 0xc000000c00000000;
  EXPECT_EQ(r->error_cnt(), 14);
  r->container[1] = 0xc000000c00000c00;
  EXPECT_EQ(r->error_cnt(), 16);
  r->container[1] = 0xc000000c00000c01;
  EXPECT_EQ(r->error_cnt(), 17);
  r->container[1] = 0xc000000c00000c03;
  EXPECT_EQ(r->error_cnt(), 18);
  r->container[1] = 0xc000000c00000c04;
  EXPECT_EQ(r->error_cnt(), 17);
  r->container[1] = 0xc000000c00040c04;
  EXPECT_EQ(r->error_cnt(), 18);
  r->container[1] = 0xc000000c01040c04;
  EXPECT_EQ(r->error_cnt(), 19);
  r->container[1] = 0xc007000c01040c04;
  EXPECT_EQ(r->error_cnt(), 22);
}
}
} //set namespace