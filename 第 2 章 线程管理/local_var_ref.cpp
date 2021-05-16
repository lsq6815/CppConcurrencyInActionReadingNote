#include <thread>
#include <iostream>

struct func
{
    int& i;
    func(int& i_) : i(i_) {}

    void operator()() {
        for (unsigned j = 0; j < 100000u; ++j) {
            // 对悬空引用的可能访问
            std::cout << "i = " << i << std::endl;
        }
    }
};

int main() {
    int some_local_var = 0;
    func my_func(some_local_var);
    some_local_var = 1;
    // 检测是否是引用
    std::cout << my_func.i << std::endl;
    std::thread my_thread(my_func);
    // 不等待线程完成
    my_thread.detach();
}