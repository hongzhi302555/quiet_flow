#include "iostream"
#include <sstream>
#include <unordered_set>

#include "head/executor.h"
#include "head/node.h"
#include "head/schedule.h"
#include "head/aspect.h"
#include "head/queue/free_lock.h"

DECLARE_bool(enable_qf_dump_graph);

namespace quiet_flow{

class SelfQueue {
  public:
    SelfQueue(size_t parallel_, size_t queue_size_):limit_queue(queue_size_), m_count(-1*parallel_) {
    }
    ~SelfQueue() {
    }

    bool try_enqueue(Node* node) {
        int32_t old_count = m_count.fetch_add(1, std::memory_order_release);
        if (old_count >= 0) {
            return limit_queue.try_enqueue(node);
        }
        return false;
    }

    bool try_dequeue(Node** node) {
        int32_t old_count = m_count.fetch_sub(1, std::memory_order_release);
        if (old_count > 0) {
            while(!limit_queue.try_dequeue((void**)node));
            return true;
        }
        return false;
    }

  private:
    queue::free_lock::LimitQueue limit_queue;
    std::atomic<int32_t> m_count;
    std::mutex mutex_;
};

void Graph::ready(Node* node) {
    if (limit_graph && self_queue->try_enqueue(node)) {
        return;
    }
    Schedule::add_new_task(node);
}

Node* Graph::finish(std::vector<Node*>& notified_nodes) {
    Node* next_task = nullptr;

    if (!limit_graph) {
        if (notified_nodes.empty()) {
            return nullptr;
        }

        auto* temp_node = notified_nodes[0];
        uint64_t i=1;
        while (i < notified_nodes.size()) {
            auto node = notified_nodes[i];
            i ++;
            if (!node->parent_graph->limit_graph) {
                next_task = node;
                break;
            }
            node->parent_graph->ready(node); 
        }
        while (i < notified_nodes.size()) {
            auto node = notified_nodes[i];
            i ++;
            node->parent_graph->ready(node); 
        }

        if (next_task) {
            temp_node->parent_graph->ready(temp_node); 
            return next_task;
        }
        if (!temp_node->parent_graph->limit_graph) {
            return temp_node;
        }
        if (!temp_node->parent_graph->self_queue->try_enqueue(temp_node)) {
            return temp_node;
        }
        return nullptr;
    }

    if (notified_nodes.empty()) {
        self_queue->try_dequeue(&next_task);
        return next_task;
    }

    auto* temp_node = notified_nodes[0];
    uint64_t i=1;
    while (i < notified_nodes.size()) {
        auto node = notified_nodes[i];
        i ++;
        if (node->parent_graph == this) {
            next_task = node;
            break;
        }
        node->parent_graph->ready(node); 
    }
    while (i < notified_nodes.size()) {
        auto node = notified_nodes[i];
        i ++;
        node->parent_graph->ready(node); 
    }

    if (next_task) {
        temp_node->parent_graph->ready(temp_node);
        return next_task;
    }
    if (temp_node->parent_graph == this) {
        return temp_node;
    }
    if (self_queue->try_dequeue(&next_task)) {
        temp_node->parent_graph->ready(temp_node);
        return next_task;
    }
    return temp_node;
}

const char Graph::fast_node_max_num = 63;  // node.fast_downstrem (long int 最高位是 flag)

Graph::Graph(Node* p):idx(0), parent_node(p), node_num(0){
    limit_graph = false;
    status = RunningStatus::Initing;
}

Graph::Graph(Node* p, size_t parallel, size_t queue_size):idx(0), parent_node(p), node_num(0) {
    if (parallel < 30 && queue_size < (1<<15)) {
        limit_graph = true;
        self_queue = new SelfQueue(parallel, queue_size);
    }
    status = RunningStatus::Initing;
}

Graph::~Graph() {
    clear_graph();
    if (limit_graph) {
        delete self_queue;
    }
    status = RunningStatus::Destroy;
}

void Graph::clear_graph(){
    if (FLAGS_enable_qf_dump_graph) {
        return;
    }
    if (status == RunningStatus::Initing) {
        return;
    }
    if (status == RunningStatus::Finish) {
        nodes.clear();
        fast_nodes.clear();
        return;
    }
    
    std::vector<Node*> required_nodes;
    get_nodes(required_nodes);

    if (Schedule::safe_get_cur_exec()) {
        // 必须确保是 schcedule 任务
        ScheduleAspect::require_node(required_nodes);
    }

    status = RunningStatus::Finish;
    for (auto i: required_nodes) {
        int cnt = 0;
        while (i->status != RunningStatus::Recoverable) {
            cnt ++;
            #ifdef QUIET_FLOW_DEBUG
            std::cout << (unsigned long int)i << "\n";
            #else
            if ((cnt % 8) == 0) {
                usleep(1);
            }
            #endif
        }
        i->mutex_.lock();   // 保证高并发下，set_status(Recoverable)
        i->mutex_.unlock();
    }
    nodes.clear();
    fast_nodes.clear();
}

void Graph::get_nodes(std::vector<Node*>& required_nodes) {
    required_nodes.reserve(nodes.size() + fast_node_max_num);

    mutex_.lock();
    for (auto& n_: fast_nodes) {
        if (n_) {
            required_nodes.push_back(n_.get());
        }
    } 
    for (auto& n_: nodes) {
        required_nodes.push_back(n_.get());
    } 
    mutex_.unlock();
}

Node* Graph::get_node(uint64_t idx) {
    if (idx < fast_node_max_num) {
        return fast_nodes[idx].get();
    } else {
        mutex_.lock();
        auto t_ = nodes[idx-fast_node_max_num].get();
        mutex_.unlock();
        return t_;
    }
}

struct ReuseNode::NodeDelete {
    NodeDelete(queue::free_lock::LimitQueue& limit_queue_): limit_queue(limit_queue_){}
    queue::free_lock::LimitQueue& limit_queue;
    void operator()(Node *o) {
        o->release();
        if (limit_queue.try_enqueue(o)) {
            o->pending_worker_num_.fetch_sub(1, std::memory_order_relaxed);
        } else {
            delete o;
        }
    }
};

struct ReuseNode::Pool {
    queue::free_lock::LimitQueue limit_queue;
    ReuseNode::NodeDelete node_delete;
    Pool(size_t size): limit_queue(size), node_delete(limit_queue) {}
};

std::shared_ptr<Node> Graph::create_edges(Node* new_node, const std::vector<Node*>& required_nodes) {
    if (not new_node->is_reuse()) {
        return create_edges(std::shared_ptr<Node>(new_node), required_nodes);
    }
    auto* reuse_node = (ReuseNode*)new_node;
    return create_edges(std::shared_ptr<Node>(new_node, reuse_node->get_pool()->node_delete), required_nodes);
}

std::shared_ptr<Node> Graph::create_edges(std::shared_ptr<Node> shared_node, const std::vector<Node*>& required_nodes) {
    Node* new_node = shared_node.get();
    if (parent_node) {
        new_node->root_node = parent_node->root_node;
    }
    {
        mutex_.lock();
        status = RunningStatus::Running;
        new_node->set_parent_graph(this, node_num);
        if (node_num == 0) {
            fast_nodes.resize(fast_node_max_num);
        }
        if (node_num < fast_node_max_num) {
            fast_nodes[new_node->node_id] = shared_node;
        } else {
            nodes.push_back(shared_node);
        }
        node_num += 1;
        mutex_.unlock();
    }

    if (required_nodes.size() > 0) {
        new_node->add_wait_count(required_nodes.size());

        for (auto r_node: required_nodes) {
            if (r_node) {
                Node::append_upstreams(new_node, r_node);
                r_node->add_downstream(new_node);         
            } else {                                      
                if (1 == new_node->sub_wait_count()) {
                    new_node->parent_graph->ready(new_node);
                }                                         
            }   
        }
    } else {
        new_node->parent_graph->ready(new_node);
    }

    Node::append_upstreams(new_node, Node::flag_node);

    return shared_node;
}

ReuseNode::Pool* ReuseNode::init_pool(int size) {
    return new ReuseNode::Pool(size);
}

ReuseNode* ReuseNode::get_from_pool(ReuseNode::Pool* pool) {
    ReuseNode* node = nullptr;
    if (pool->limit_queue.try_dequeue((void**)&node)) {
        #ifdef QUIET_FLOW_DEBUG
        std::cout << "LambdaNode reuse" << std::endl;
        #endif 
        node->create();
        node->pending_worker_num_.fetch_add(1, std::memory_order_relaxed);
    }
    return node;
}

class LambdaNode: public ReuseNode {
  private:
    LambdaNode(std::function<void(Graph*)> &&callable, std::string debug_name="")
    : lambda_holder([sub_graph=this->get_graph(), callable=std::move(callable)]() {
        callable(sub_graph);
    }) {
      #ifdef QUIET_FLOW_DEBUG
      name_for_debug = debug_name;
      std::cout << "LambdaNode create" << std::endl;
      #endif
    }
    ~LambdaNode() {
      #ifdef QUIET_FLOW_DEBUG
      std::cout << "LambdaNode delete" << std::endl;
      #endif
    }
  private:
    static ReuseNode::Pool* pool;
  public:
    static ReuseNode* New(std::function<void(Graph*)> &&callable, std::string debug_name) {
        auto node = (LambdaNode*)(get_from_pool(pool));
        if (node) {
            #ifdef QUIET_FLOW_DEBUG
            node->name_for_debug = debug_name;
            #endif
            node->lambda_holder = [sub_graph=node->get_graph(), callable=std::move(callable)]() {callable(sub_graph);};
        } else {
            node = new LambdaNode(std::move(callable), debug_name);
        }
        return node;
    }

  protected:
    virtual void run(){
      lambda_holder();
      ScheduleAspect::wait_graph(this->get_graph());
    }
    virtual Pool* get_pool() override {
      return pool;
    };
public:
    std::function<void()> lambda_holder;
};

ReuseNode::Pool* LambdaNode::pool = ReuseNode::init_pool(1<<14);

std::shared_ptr<Node> Graph::create_edges(std::function<void(Graph*)> &&callable, const std::vector<Node*>& required_nodes, std::string debug_name) {
    ReuseNode* node = LambdaNode::New(std::move(callable), debug_name);
    return create_edges(std::shared_ptr<Node>(node, node->get_pool()->node_delete), required_nodes);
}

void Node::set_status(RunningStatus s) {
    mutex_.lock();
    status = s;
    mutex_.unlock();
}

int Node::add_downstream(Node* node) {
    if (this == node) {
        if (1 == node->sub_wait_count()) {
            node->parent_graph->ready(node);
        }
        return -1;
    }

    if (status >= RunningStatus::Finish) {
        if (1 == node->sub_wait_count()) {
            node->parent_graph->ready(node);
        }
        return -1;
    }

    if (parent_graph == node->parent_graph) {
        // 判断是否加入 fast_down_streams
        if (node->node_id < Graph::fast_node_max_num) {
            // 防止重复加入
            if (bit_map_get(fast_down_strams, node->node_id, Graph::fast_node_max_num)) {
                if (1 == node->sub_wait_count()) {
                    node->parent_graph->ready(node);
                }
            } else {
                bit_map_set(fast_down_strams, node->node_id, Graph::fast_node_max_num);
            }

            long int f_ = 0;
            mutex_.lock();
            f_ = fast_down_strams_bak;
            mutex_.unlock();
            if (f_ == Node::fast_down_strams_bak_init) {
                // 本节点未结束
                return 0;
            }
            if (!bit_map_get(f_, node->node_id, Graph::fast_node_max_num)) {
                if (1 == node->sub_wait_count()) {
                    node->parent_graph->ready(node);
                }
                return -1;
            }
            return 0;
        }
    }
    
    mutex_.lock();
    if (status >= RunningStatus::Finish) {
        mutex_.unlock();
        if (1 == node->sub_wait_count()) {
            node->parent_graph->ready(node);
        }
        return -1;
    }
    down_streams.push_back(node);
    mutex_.unlock();

    return 0;
}

void Node::append_upstreams(Node* self_node, const Node* node) {
    #ifdef QUIET_FLOW_DEBUG
    self_node->up_streams.push_back(node);
    #else
    if (FLAGS_enable_qf_check_circle || FLAGS_enable_qf_dump_graph) {
        self_node->up_streams.push_back(node);
    }
    #endif
}

std::string Graph::dump(bool is_root) {
    if (!FLAGS_enable_qf_dump_graph) {
        return "";
    }

    static const std::string wait_edge = "[color=black]";
    static const std::string wait_edge1 = "[color=blue]";
    static const std::string wait_edge2 = "[color=red]";
    static const std::string wait_edge3 = "[color=green]";
    static const std::string back_edge = "[style=dashed, color=gray]";
    static const std::string nowait_edge = "[color=gray]";
    static const std::string virtual_edge = "[style=dotted, color=gray]";

    std::vector<Node*> required_nodes;
    get_nodes(required_nodes);

    // 将所有节点分类到子图
    std::unordered_map<long int, std::unordered_set<const Node*>> sub_graph_map;
    for (auto node: required_nodes) {
        if (sub_graph_map.find((long int)node->parent_graph) == sub_graph_map.end()) {
            std::unordered_set<const Node*> t_;
            t_.emplace(node);
            sub_graph_map.emplace((long int)node->parent_graph, t_);
        } else {
            sub_graph_map[(long int)node->parent_graph].emplace(node);
        }
    }

    std::unordered_map<std::string, bool> edge_back_flag_map;
    std::unordered_map<std::string, std::string> edge_color_map;
    for (auto node: required_nodes) {
        bool need_back_node = false;
        for (auto& up_node: node->up_streams) {
            if (up_node == Node::flag_node) {
                need_back_node = true;
                continue;
            }
            if (sub_graph_map.find((long int)node->parent_graph) == sub_graph_map.end()) {
                std::unordered_set<const Node*> t_;
                t_.emplace(up_node);
                sub_graph_map.emplace((long int)up_node->parent_graph, t_);
            } else {
                sub_graph_map[(long int)up_node->parent_graph].emplace(up_node);
            }

            std::ostringstream oss_edge;
            if (up_node->require_sub_graph) {
                // 画图缀个 1
                oss_edge << (long int)up_node << "1" << "->" << (long int)node;
            } else {
                oss_edge << (long int)up_node << "->" << (long int)node;
            }
            edge_color_map[oss_edge.str()] = nowait_edge;
            edge_back_flag_map[oss_edge.str()] = need_back_node;
        }
    }

    // 调整依赖边颜色
    for (auto node: required_nodes) {
        std::ostringstream oss_start_edge;
        oss_start_edge << (long int)node->parent_graph << "->" << (long int)node;
        edge_color_map[oss_start_edge.str()] = virtual_edge;

        std::vector<uint64_t> idx_vec;
        bit_map_idx(node->fast_down_strams_bak, Graph::fast_node_max_num, idx_vec);
        for (auto idx: idx_vec) {
            auto down_node = node->parent_graph->get_node(idx);
            std::ostringstream oss_edge;
            if (node->require_sub_graph) {
                // 画图缀个 1
                oss_edge << (long int)node << "1" << "->" << (long int)down_node;
            } else {
                oss_edge << (long int)node << "->" << (long int)down_node;
            }
            edge_color_map[oss_edge.str()] = wait_edge;
        }
        for (auto& down_node: node->down_streams) {
            if (down_node->parent_graph != node->parent_graph) {
                continue;
            }
            std::ostringstream oss_edge;
            if (node->require_sub_graph) {
                // 画图缀个 1
                oss_edge << (long int)node << "1" << "->" << (long int)down_node;
            } else {
                oss_edge << (long int)node << "->" << (long int)down_node;
            }
            edge_color_map[oss_edge.str()] = wait_edge1;
            // edge_color_map[oss_edge.str()] = wait_edge;
        }
    }

    std::ostringstream oss;
    if (is_root) {
        oss << "digraph QuietFlow {\n";
    }
    for (auto& sub_graph: sub_graph_map) {
        oss << "\nsubgraph cluster_" << sub_graph.first << " {\ncolor=black;\n";
        oss << sub_graph.first << "[label=start, shape=diamond];\n";
        for (auto& sub_node: sub_graph.second) {
            long int sub_node_id = (long int)sub_node;
            if (sub_node->require_sub_graph) {
                oss << "\nsubgraph cluster_" << sub_node_id << " {\nstyle=filled;\ncolor=lightgray;\n";
                oss << sub_node_id << 1 << "[label=" << sub_node->node_debug_name("--end") << "]\n";
                oss << sub_node_id << "->" << sub_node_id << 1 << wait_edge << ";\n";
                // oss << sub_node_id << "->" << sub_node_id << 1 << wait_edge2 << ";\n";
                oss << sub_node_id << "[label=" << sub_node->node_debug_name() << "]\n";
                oss << "}\n";
            } else {
                oss << sub_node_id << "[label=" << sub_node->node_debug_name() << "]\n";
            }
        }
        oss << "}\n";

        if (parent_node) {
            oss << (long int)parent_node << "->" << sub_graph.first << wait_edge << ";\n";
            // oss << (long int)parent_node << "->" << sub_graph.first << wait_edge3 << ";\n";
        }
    }
    for (auto& edge_color: edge_color_map) {
        if (edge_back_flag_map[edge_color.first]) {
            oss << edge_color.first << "1" << back_edge << ";\n";
        } else {
            oss << edge_color.first << edge_color.second + ";\n";
        }
    }

    for (auto node: required_nodes) {
        oss << node->sub_graph->dump(false);
    }
    if (is_root) {
        oss << "}\n";
    }
    return oss.str();

    return "";
}
}