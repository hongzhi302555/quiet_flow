cc_library(
    name="quiet_flow_lib",
    srcs=[
        "src/*.cpp",
    ],
    incs=[
        "",
    ],
    deps=[
        "cpp3rdlib/folly:v2018.08.20.00@//cpp3rdlib/folly:folly",
        "cpp3rdlib/concurrentqueue:master@//cpp3rdlib/concurrentqueue:concurrentqueue"
    ],
    extra_cppflags = [
        "-std=c++14",
        "-O2",
        "-g",
        "-fopenmp",
        #"-D QUIET_FLOW_DEBUG",
        #"-D QUIET_FLOW_DUMP_GRAPH",
        "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-local-typedefs",
        "-Wno-unused-function",
        "-Wno-unused-parameter",
        "-Wunused-variable",
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
    ],
    extra_cppflags = [
        "-std=c++14",
        "-O2",
        "-g",
        "-fopenmp",
        "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-local-typedefs",
        "-Wno-unused-function",
        "-Wno-unused-parameter",
        "-Wunused-variable",
    ],
    bundle_path="/home/yanghongzhi/work/quiet_flow_dev",
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
        "cpp3rdlib/gflags:2.1.2@//cpp3rdlib/gflags:gflags",
        "cpp3rdlib/murmurhash3@//cpp3rdlib/murmurhash3:murmurhash3",
        "cpp3rdlib/glog:0.3.3@//cpp3rdlib/glog:glog",
        "cpputil/json:1.0.0@//cpputil/json:json",
        "cpputil/log:1.0.0@//cpputil/log:log",
        "cpputil/metrics2:1.0.0@//cpputil/metrics2:metrics2",
        "cpputil/program:1.0.0@//cpputil/program:conf",
    ],
    extra_cppflags = [
        "-std=c++14",
        "-O2",
        "-g",
        "-fopenmp",
        "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-local-typedefs",
        "-Wno-unused-function",
        "-Wno-unused-parameter",
        "-Wunused-variable",
    ],
    bundle_path="/home/yanghongzhi/work/quiet_flow_dev",
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
        "cpp3rdlib/gflags:2.1.2@//cpp3rdlib/gflags:gflags",
        "cpp3rdlib/murmurhash3@//cpp3rdlib/murmurhash3:murmurhash3",
        "cpp3rdlib/glog:0.3.3@//cpp3rdlib/glog:glog",
        "cpputil/json:1.0.0@//cpputil/json:json",
        "cpputil/log:1.0.0@//cpputil/log:log",
        "cpputil/metrics2:1.0.0@//cpputil/metrics2:metrics2",
        "cpputil/program:1.0.0@//cpputil/program:conf",
    ],
    extra_cppflags = [
        "-std=c++14",
        "-O2",
        "-g",
        "-fopenmp",
        "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-local-typedefs",
        "-Wno-unused-function",
        "-Wno-unused-parameter",
        "-Wunused-variable",
    ],
    bundle_path="/home/yanghongzhi/work/quiet_flow_dev",
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
    ],
    extra_cppflags = [
        "-std=c++14",
        "-O2",
        "-g",
        "-fopenmp",
        "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-local-typedefs",
        "-Wno-unused-function",
        "-Wno-unused-parameter",
        "-Wunused-variable",
    ],
    bundle_path="/home/yanghongzhi/work/quiet_flow_dev",
)

cc_binary(
    name="debug_stability",
    srcs=[
        "benchmark/stability.cpp",
    ],
    incs=[
        "",
    ],
    deps=[
        "#asan",
        ":quiet_flow_lib",
    ],
    extra_cppflags = [
        "-std=c++14",
        "-O2",
        "-g",
        "-fopenmp",
        "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-local-typedefs",
        "-Wno-unused-function",
        "-Wno-unused-parameter",
        "-Wunused-variable",
        "-fsanitize=address",
    ],
    bundle_path="/home/yanghongzhi/work/quiet_flow_dev",
)
