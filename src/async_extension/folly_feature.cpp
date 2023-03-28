#include <folly/futures/Future.h>
#include <cxxabi.h>
#include "iostream"

#include "head/async_extension/folly_future.h"

namespace quiet_flow{

FollyFutureAspect::Assistant* FollyFutureAspect::aspect_ = new FollyFutureAspect::Assistant();

class InnerEndNode: public Node {
  public:
    InnerEndNode(const std::string& debug_name="") {
        #ifdef QUIET_FLOW_DEBUG
        name_for_debug = "end_node@" + debug_name;
        #endif
    }
    ~InnerEndNode() {
    }
    void run() {
        #ifdef QUIET_FLOW_DEBUG
        StdOut() << name_for_debug;
        #endif
    }
};

Node* FollyFutureAspect::Assistant::make_future(const std::string& debug_name) {
    return (new InnerEndNode(debug_name));
}
}