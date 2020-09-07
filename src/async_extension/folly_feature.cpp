#include <folly/futures/Future.h>
#include <cxxabi.h>
#include "iostream"

#include "head/async_extension/folly_future.h"

namespace quiet_flow{

class FutureInnerNode: public Node {
  public:
    FutureInnerNode(const std::string& debug_name="") {
        #ifdef QUIET_FLOW_DEBUG
        name_for_debug = "end_node@" + debug_name;
        #endif
    }
    ~FutureInnerNode() {
    }
    void run() {
        #ifdef QUIET_FLOW_DEBUG
        std::cout << name_for_debug << std::endl;
        #endif
    }
};

Node* FollyFutureNode::make_future(const std::string& debug_name) {
    return (new FutureInnerNode(debug_name));
}
}