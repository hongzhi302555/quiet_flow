#!/usr/bin/env bash

echo "================= build started ================="

cur=`pwd`
source_root=$(cd `dirname $0`; pwd)

export CXXFLAGS="-fno-omit-frame-pointer -fno-optimize-sibling-calls -fopenmp=libomp -fsanitize=address -fsanitize-address-use-after-scope -g -O0 -DBYTESAN -DBYTESAN_THREAD_STACK_SIZE=8"
export CFLAGS="-fno-omit-frame-pointer -fno-optimize-sibling-calls -fopenmp=libomp -fsanitize=address -fsanitize-address-use-after-scope -g -O0 -DBYTESAN -DBYTESAN_THREAD_STACK_SIZE=8"
export LINKFLAGS="-fsanitize=address -fopenmp=libomp"
export FILTERFLAGS="cpp3rdlib/jemalloc/lib/libjemalloc.a -Werror -Xlinker --no-undefined -O1 -O2 -O3 -O build64_release/cpp3rdlib/jemalloc/libjemalloc.so build64_release/cpp3rdlib/jemalloc/jemalloc.so"
export ASAN_SYMBOLIZER_PATH="/opt/tiger/typhoon-blade/gccs/x86_64-x86_64-clang-1101/bin/llvm-symbolizer"


export DISTCC_IO_TIMEOUT=2400
. /etc/os-release
echo "采用Clang编译"
export TOOLCHAIN=x86_64-clang1101
blade build :* --bundle=release --safe-patchelf  --update-deps --toolchain=${TOOLCHAIN} --enable-bpt