#!/usr/bin/env bash

echo "================= build started ================="

cur=`pwd`
source_root=$(cd `dirname $0`; pwd)

export DISTCC_IO_TIMEOUT=2400
. /etc/os-release

echo "采用Clang编译"
export TOOLCHAIN=x86_64-clang1101
blade build :* --bundle=release --safe-patchelf  --update-deps --toolchain=${TOOLCHAIN} --enable-bpt