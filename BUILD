custom_deps = {
    "x86_64-gcc830" : [
    ],
    "x86_64-clang1101": [
        "cpp3rdlib/msgpack:3.0.1-3@//cpp3rdlib/msgpack:msgpackc",
        "cpp3rdlib/zlib:1.2.11@//cpp3rdlib/zlib:z",
        "cpp3rdlib/glog:0.4.0@//cpp3rdlib/glog:glog",
        "cpp3rdlib/gflags:2.2.2@//cpp3rdlib/gflags:gflags",
        "cpp3rdlib/bzip2:1.0.8@//cpp3rdlib/bzip2:bz2",
        "cpp3rdlib/googlemock:1.10.0@//cpp3rdlib/googlemock:gmock",
        "cpp3rdlib/gtest:1.10.0@//cpp3rdlib/gtest:gtest",
        "cpp3rdlib/jemalloc:sys-5.2.1-thp-off@//cpp3rdlib/jemalloc:jemalloc",
    ]
}

cc_library(
    name="quiet_flow_lib",
    srcs=[
        "src/*.cpp",
        "src/*/*.cpp",
    ],
    incs=[
        "./",
    ],
    export_incs=["./"],
    deps=[
        "cpp3rdlib/folly:v2018.08.20.00@//cpp3rdlib/folly:folly",
        "cpp3rdlib/concurrentqueue:master@//cpp3rdlib/concurrentqueue:concurrentqueue"
    ],
    extra_cppflags = [
        "-O3", "-g",
        "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-local-typedefs", "-Wno-unused-function",
        "-Wno-unused-parameter", "-Wno-unused-variable",
        "-Wno-comment", "-ftemplate-depth=9000",
        "-fopenmp",
        # "-D QUIET_FLOW_DEBUG",
    ],
)

cc_binary(
    name="demo",
    srcs=[
        "test/demo.cpp",
    ],
    incs=[
        "",
    ],
    deps=[
        ":quiet_flow_lib",
        "cpputil/metrics2:1.0.0@//cpputil/metrics2:metrics2",
    ],
    custom_deps=custom_deps,
    extra_cppflags = [
        "-O3", "-g",
        "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-local-typedefs", "-Wno-unused-function",
        "-Wno-unused-parameter", "-Wno-unused-variable",
        "-Wno-comment", "-ftemplate-depth=9000",
        "-fopenmp"
    ],
    bundle_path="lib",
)

cc_binary(
    name="fibonacci_dynamic",
    srcs=[
        "benchmark/fibonacci_dynamic.cpp",
    ],
    incs=[
        "",
    ],
    deps=[
        ":quiet_flow_lib",
        "cpputil/common:1.0.0@//cpputil/common:common_utils",
        "cpp3rdlib/murmurhash3@//cpp3rdlib/murmurhash3:murmurhash3",
        "cpp3rdlib/glog:0.3.3@//cpp3rdlib/glog:glog",
        "cpputil/json:1.0.0@//cpputil/json:json",
        "cpputil/log:1.0.0@//cpputil/log:log",
        "cpputil/metrics2:1.0.0@//cpputil/metrics2:metrics2",
        "cpputil/program:1.0.0@//cpputil/program:conf",
    ],
    custom_deps=custom_deps,
    extra_cppflags = [
        "-O3", "-g",
        "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-local-typedefs", "-Wno-unused-function",
        "-Wno-unused-parameter", "-Wno-unused-variable",
        "-Wno-comment", "-ftemplate-depth=9000",
        "-fopenmp"
    ],
    bundle_path="lib",
)

cc_binary(
    name="fibonacci_static",
    srcs=[
        "benchmark/fibonacci_static.cpp",
    ],
    incs=[
        "",
    ],
    deps=[
        ":quiet_flow_lib",
        "cpputil/common:1.0.0@//cpputil/common:common_utils",
        "cpp3rdlib/murmurhash3@//cpp3rdlib/murmurhash3:murmurhash3",
        "cpp3rdlib/glog:0.3.3@//cpp3rdlib/glog:glog",
        "cpputil/json:1.0.0@//cpputil/json:json",
        "cpputil/log:1.0.0@//cpputil/log:log",
        "cpputil/metrics2:1.0.0@//cpputil/metrics2:metrics2",
        "cpputil/program:1.0.0@//cpputil/program:conf",
    ],
    custom_deps=custom_deps,
    extra_cppflags = [
        "-O3", "-g",
        "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-local-typedefs", "-Wno-unused-function",
        "-Wno-unused-parameter", "-Wno-unused-variable",
        "-Wno-comment", "-ftemplate-depth=9000",
        "-D QUIET_FLOW_DEBUG",
        "-fopenmp"
    ],
    bundle_path="lib",
)

cc_binary(
    name="stability",
    srcs=[
        "benchmark/stability.cpp",
    ],
    incs=[
        "",
    ],
    deps=[
        ":quiet_flow_lib",
        "cpputil/metrics2:1.0.0@//cpputil/metrics2:metrics2",
    ],
    custom_deps=custom_deps,
    extra_cppflags = [
        "-O3", "-g",
        "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-local-typedefs", "-Wno-unused-function",
        "-Wno-unused-parameter", "-Wno-unused-variable",
        "-Wno-comment", "-ftemplate-depth=9000",
        "-fopenmp"
    ],
    bundle_path="lib",
)

cc_test(
    name="_unit_test",
    srcs=[
        "unit_test/*_test.cpp",
        "unit_test/*/*_test.cpp",
    ],
    incs=["."],
    extra_cppflags = [
        "-O3", "-g",
        "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-local-typedefs", "-Wno-unused-function",
        "-Wno-unused-parameter", "-Wno-unused-variable",
        "-Wno-comment", "-ftemplate-depth=9000",
        "-D UNIT_TEST",
        "-fopenmp"
    ],
    deps=[
        ":quiet_flow_lib",
        "cpputil/metrics2:1.0.0@//cpputil/metrics2:metrics2",
        "cpp3rdlib/gtest:1.6.0@//cpp3rdlib/gtest:gtest",
    ],
    bundle_path="lib",
)
    