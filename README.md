[README](https://bbd1vqv6zo.feishu.cn/docx/L0rcdNRqToj3HvxqPkLc3awnnpe)

[设计文档](https://bbd1vqv6zo.feishu.cn/docx/TrzodDAQeogff9xgfKScYVVPnBc)

# 快速上手
```
make

# find -maxdepth 1 -executable 
./bin/demo ./bin/stability ./bin/fibonacci_static ./bin/fibonacci_dynamic ./bin/_unit_test
```

# demo 工具
```
// data-arch/quiet_flow/test/demo.cpp
int main(int argc, char** argv) {
    // 使用 2 个 thread run 图
    quiet_flow::Schedule::init(2);

    quiet_flow::Graph g(nullptr);
    g.create_edges(new quiet_flow::test::NodeM("m"), {});
    quiet_flow::Node::block_thread_for_group(&g);
    g.clear_graph();
    
    std::cout << g.dump(true);

    quiet_flow::Schedule::destroy();
    return 0;
}
```
![image](https://user-images.githubusercontent.com/7760049/236439174-8088797e-72c2-46b8-a6b1-1a224771dd68.png)

1. 初始化一个 schedule
2. 向 graph 中丢入 NodeM (随丢随 run)
3. NodeM 内部可以构建一些子图，以及一些 rpc
4. 执行完毕，清理善后
