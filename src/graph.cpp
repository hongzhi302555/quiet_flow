#include "iostream"
#include <sstream>
#include <unordered_set>

#include "head/executor.h"
#include "head/node.h"
#include "head/schedule.h"

DECLARE_bool(enable_qf_dump_graph);

namespace quiet_flow{
const size_t Graph::fast_node_max_num = 63;  // node.fast_downstrem (long int 最高位是 flag)

Graph::Graph(Node* p):idx(0), parent_node(p), node_num(0){
    fast_nodes.resize(fast_node_max_num);
}

Graph::~Graph() {
    clear_graph();
}

void Graph::clear_graph(){
    if (FLAGS_enable_qf_dump_graph) {
        return;
    }

    std::vector<Node*> required_nodes;
    get_nodes(required_nodes);

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
        i->_mutex.lock();   // 保证高并发下，set_status(Recoverable)
        i->_mutex.unlock();
    }
    nodes.clear();
    fast_nodes.clear();
}

void Graph::get_nodes(std::vector<Node*>& required_nodes) {
    _mutex.lock();
    for (auto& n_: fast_nodes) {
        if (n_) {
            required_nodes.push_back(n_.get());
        }
    } 
    for (auto& n_: nodes) {
        required_nodes.push_back(n_.get());
    } 
    _mutex.unlock();
}

Node* Graph::get_node(size_t idx) {
    if (idx < fast_node_max_num) {
        return fast_nodes[idx].get();
    } else {
        _mutex.lock();
        return nodes[idx-fast_node_max_num].get();
        _mutex.unlock();
    }
}

std::shared_ptr<Node> Graph::create_edges(Node* new_node, const std::vector<Node*>& required_nodes) {
    std::shared_ptr<Node> shared_node;
    shared_node.reset(new_node);
    {
        _mutex.lock();
        new_node->set_parent_graph(this, node_num);
        node_num += 1;
        if (node_num < fast_node_max_num) {
            fast_nodes[new_node->node_id] = shared_node;
        } else {
            nodes.push_back(shared_node);
        }
        _mutex.unlock();
    }

    if (required_nodes.size() > 0) {
        new_node->add_wait_count(required_nodes.size());

        for (auto r_node: required_nodes) {
            new_node->append_upstreams(r_node);
            r_node->add_downstream(new_node);
        }
    } else {
        Schedule::add_new_task(new_node);
    }
    new_node->append_upstreams(Node::flag_node);

    return shared_node;
}

void Node::set_status(RunningStatus s) {
    _mutex.lock();
    status = s;
    _mutex.unlock();
}

int Node::add_downstream(Node* node) {
    if (this == node) {
        if (1 == node->sub_wait_count()) {
            Schedule::add_new_task(node);
        }
        return -1;
    }

    if (status >= RunningStatus::Finish) {
        if (1 == node->sub_wait_count()) {
            Schedule::add_new_task(node);
        }
        return -1;
    }

    if (parent_graph == node->parent_graph) {
        // 判断是否加入 fast_down_streams
        if (node->node_id < Graph::fast_node_max_num) {
            bit_map_set(fast_down_strams, node->node_id, Graph::fast_node_max_num);
            long int f_ = 0;
            _mutex.lock();
            f_ = fast_down_strams_bak;
            _mutex.unlock();

            if (f_ == Node::fast_down_strams_bak_init) {
                // 本节点未结束
                return 0;
            }
            if (!bit_map_get(f_, node->node_id, Graph::fast_node_max_num)) {
                if (1 == node->sub_wait_count()) {
                    Schedule::add_new_task(node);
                }
                return -1;
            }
            return 0;
        }
    }
    
    _mutex.lock();
    if (status >= RunningStatus::Finish) {
        _mutex.unlock();
        if (1 == node->sub_wait_count()) {
            Schedule::add_new_task(node);
        }
        return -1;
    }
    down_streams.push_back(node);
    _mutex.unlock();

    return 0;
}

void Node::append_upstreams(const Node* node) {
    if (FLAGS_enable_qf_check_circle || FLAGS_enable_qf_dump_graph) {
        up_streams.push_back(node);
    }
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

        std::vector<size_t> idx_vec;
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