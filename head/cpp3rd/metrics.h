#pragma once

#ifdef LOCAL_METRICS

namespace quiet_flow{
class Metrics {
  public:
    static int init(const std::string& prefix, const std::string& psm, int batch_size = 128, int auto_batch = 1) {
	return 1;
    }
    static int emit_counter(const std::string& name, double value) {
       return 1;
    }
    static int emit_counter(const std::string& name, double value, const std::string& tagkv) {
       return 1;
    }
    static int emit_timer(const std::string& name, double value) {
       return 1;
    }
    static int emit_timer(const std::string& name, double value, const std::string& tagkv) {
       return 1;
    }
};
}


#else

#include "cpputil/metrics2/metrics.h"

namespace quiet_flow{
class Metrics: public cpputil::metrics2::Metrics {
};
}

#endif