AR = /opt/tiger/typhoon-blade/gccs/x86_64-x86_64-clang-1101/bin/llvm-ar rcs
CXX = /opt/tiger/typhoon-blade/gccs/x86_64-x86_64-clang-1101/bin/clang++
CXXFLAGS = -c -std=c++14 -fPIC -O3 -g \
	--sysroot=/opt/tiger/typhoon-blade/gccs/x86_64-x86_64-gcc-830/sysroot \
	--gcc-toolchain=/opt/tiger/typhoon-blade/gccs/x86_64-x86_64-gcc-830 
LINKFLAGS = --sysroot=/opt/tiger/typhoon-blade/gccs/x86_64-x86_64-gcc-830/sysroot \
	--gcc-toolchain=/opt/tiger/typhoon-blade/gccs/x86_64-x86_64-gcc-830 \
	-ldl -lpthread -latomic -fuse-ld=lld \
	-Wl,-dynamic-linker,/home/yanghongzhi/work/quiet_flow_dev/build64_release/quiet_flow/lib/ld-linux-x86-64.so.2 \
	-Wl,-rpath=/home/yanghongzhi/work/quiet_flow_dev/build64_release/quiet_flow/lib \
	-L/opt/tiger/typhoon-blade/gccs/x86_64-x86_64-gcc-830/sysroot/usr/lib64
DYNAMIC_LIB = ../build64_release/cpp3rdlib/gflags/libgflags.so 

INCLS = -I. -I../build64_release -I../cpp3rdlib/gflags/include  

SRC_FILES = $(shell find ./src -path ./src/async_extension  -prune -o -name *.cpp -print) 
LIB_TARGET = $(SRC_FILES:.cpp=.o) 
ASSEMBLY_FILES = $(shell find ./src -name *.S) 
LIB_TARGET += $(ASSEMBLY_FILES:.S=.o)

qf.a: $(LIB_TARGET)
	$(AR) $@ $^
  
.PHONY: folly_dep
folly_dep: 
	$(eval INCLS += -I../cpp3rdlib/folly/include -I../cpp3rdlib/zstd/include -I../cpp3rdlib/boost/include -I../cpp3rdlib/libevent/include -I../cpp3rdlib/double-conversion/include -I../cpp3rdlib/glog/include -I../cpp3rdlib/gflags/include -I../cpp3rdlib/openssl/include -I../cpp3rdlib/icu/include -I../cpp3rdlib/lz4/include -I../cpp3rdlib/unwind/include -I../cpp3rdlib/lzma/include -I../cpp3rdlib/zlib/include -I../cpp3rdlib/bzip2/includ)
	$(eval DYNAMIC_LIB += ../build64_release/cpp3rdlib/glog/libglog.so ../build64_release/cpp3rdlib/unwind/libunwind.so ../build64_release/cpp3rdlib/lzma/liblzma.so ../build64_release/cpp3rdlib/zlib/libz.so)
	$(eval DYNAMIC_LIB += ../cpp3rdlib/folly/lib/libfolly.a ../cpp3rdlib/zstd/lib/libzstd.a ../cpp3rdlib/boost/lib/libboost_chrono.a ../cpp3rdlib/boost/lib/libboost_context.a ../cpp3rdlib/boost/lib/libboost_filesystem.a ../cpp3rdlib/boost/lib/libboost_program_options.a ../cpp3rdlib/boost/lib/libboost_system.a ../cpp3rdlib/boost/lib/libboost_regex.a ../cpp3rdlib/boost/lib/libboost_thread.a ../cpp3rdlib/boost/lib/libboost_date_time.a ../cpp3rdlib/boost/lib/libboost_atomic.a ../cpp3rdlib/libevent/lib/libevent.a ../cpp3rdlib/double-conversion/lib/libdouble-conversion.a ../cpp3rdlib/openssl/lib/libssl.a ../cpp3rdlib/openssl/lib/libcrypto.a ../cpp3rdlib/icu/lib/libicui18n.a ../cpp3rdlib/icu/lib/libicuuc.a ../cpp3rdlib/icu/lib/libicudata.a ../cpp3rdlib/lz4/lib/liblz4.a ../cpp3rdlib/bzip2/lib/libbz2.a)
	touch folly_dep

FOLLY_EXTENSION = ./src/async_extension/folly_future.o
qf_folly.a: folly_dep $(LIB_TARGET) $(FOLLY_EXTENSION)
	$(AR) $@ $^ 

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLS) -c $^ -o $@
%.o: %.S
	$(CXX) $(CXXFLAGS) $(INCLS) -c $^ -o $@

.PHONY: run_bin
%.bin: folly_dep qf_folly.a %.o
	$(CXX) $(LINKFLAGS) $^ $(DYNAMIC_LIB) -o bin/$(notdir $(basename $@))
TEST_FILES = $(shell find ./test -name *.cpp -print) 
TEST_FILES += $(shell find ./benchmark -name *.cpp -print) 
run_bin: $(TEST_FILES:.cpp=.bin)
	mkdir -p bin
	echo "build test success"

.PHONY: unit_test 
.PHONY: gtest_dep
gtest_dep: folly_dep
	$(eval CXXFLAGS += -DUNIT_TEST)
	$(eval INCLS += -I../cpp3rdlib/gtest/include)
	$(eval DYNAMIC_LIB += ../cpp3rdlib/gtest/lib/libgtest.a ../cpp3rdlib/gtest/lib/libgtest_main.a)
	touch gtest_dep
UNIT_TEST_FILES = $(shell find ./unit_test -name *.cpp -print) 
unit_test: gtest_dep qf_folly.a $(UNIT_TEST_FILES:.cpp=.o)
	mkdir -p bin
	$(CXX) $(LINKFLAGS) $^ $(DYNAMIC_LIB) -o bin/$@

.PHONY: clean
clean:
	rm -f $(LIB_TARGET) $(FOLLY_EXTENSION) $(TEST_FILES:.cpp=.o) $(UNIT_TEST_FILES:.cpp=.o)
	rm -f qf.a qf_folly.a folly_dep gtest_dep
	rm -rf bin
