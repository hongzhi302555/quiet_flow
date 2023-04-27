include gtest.mk
include gflags.mk
include folly.mk

.DEFAULT_GOAL := run_bin

AR = ar rcs
CXX = g++
CXXFLAGS = -c -O3 -g
LINKFLAGS = -ldl -lpthread -latomic

DYNAMIC_LIB = -Lcpp3rdlib/gflags/lib/ -lgflags -Wl,-rpath=cpp3rdlib/gflags/lib/

INCLS = -I. -Icpp3rdlib/gflags/include

SRC_FILES = $(shell find ./src -path ./src/async_extension  -prune -o -name *.cpp -print) 
LIB_TARGET = $(SRC_FILES:.cpp=.o) 
ASSEMBLY_FILES = $(shell find ./src -name *.S) 
LIB_TARGET += $(ASSEMBLY_FILES:.S=.o)

qf.a: $(LIB_TARGET)
	$(AR) $@ $^
  
.PHONY: folly_dep
# folly_dep: libglog libevent libdoubleconversion
# 	$(eval INCLS += -Icpp3rdlib/folly/include -Icpp3rdlib/boost/include -Icpp3rdlib/glog/include -Icpp3rdlib/double-conversion/include)
# 	$(eval DYNAMIC_LIB += cpp3rdlib/glog/lib/libglog.so)
# 	$(eval DYNAMIC_LIB += cpp3rdlib/folly/lib/libfolly.a cpp3rdlib/boost/lib/libboost_context.a cpp3rdlib/libevent/lib/libevent.a cpp3rdlib/double-conversion/lib/libdoubleconversion.a)
folly_dep:
	$(eval CXXFLAGS += -DQUIET_FLOW_FAKE_FOLLY)

FOLLY_EXTENSION = ./src/async_extension/folly_future.o
qf_folly.a: folly_dep inner/qf_folly.a
inner/qf_folly.a: $(LIB_TARGET) $(FOLLY_EXTENSION)
	$(AR) $(notdir $@) $^ 

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLS) -c $^ -o $@
%.o: %.S
	$(CXX) $(CXXFLAGS) $(INCLS) -c $^ -o $@

.PHONY: run_bin
%.bin: %.o qf_folly.a 
	mkdir -p bin
	$(CXX) $(LINKFLAGS) $^ $(DYNAMIC_LIB) -o bin/$(notdir $(basename $@))
TEST_FILES = $(shell find ./test -name *.cpp -print) 
TEST_FILES += $(shell find ./benchmark -name *.cpp -print) 
run_bin: libgflags folly_dep $(TEST_FILES:.cpp=.bin)
	cp -r cpp3rdlib bin
	echo "build test success"

.PHONY: unit_test 
.PHONY: gtest_dep
gtest_dep: folly_dep libgtest
	$(eval CXXFLAGS += -DUNIT_TEST)
	$(eval INCLS += -Icpp3rdlib/gtest/include)
	$(eval DYNAMIC_LIB += cpp3rdlib/gtest/libgtest.a)
UNIT_TEST_FILES = $(shell find ./unit_test -name *.cpp -print) 
unit_test: gtest_dep _unit_test
_unit_test: gtest_dep qf_folly.a $(UNIT_TEST_FILES:.cpp=.o)
	mkdir -p bin
	$(CXX) $(LINKFLAGS) $^ $(DYNAMIC_LIB) -o bin/$@

.PHONY: clean
clean:
	rm -f $(LIB_TARGET) $(FOLLY_EXTENSION) $(TEST_FILES:.cpp=.o) $(UNIT_TEST_FILES:.cpp=.o)
	rm -f qf.a qf_folly.a
	rm -rf bin
