#include <thread>
#include <iostream>

class thread_guard
{
private:
    std::thread& t;
public:
    explicit thread_guard(std::thread& t_) : t(t_) {}

    ~thread_guard() {
        if (t.joinable()) 
            t.join();
    }

    thread_guard(thread_guard const&) = delete;
    thread_guard& operator=(thread_guard const&) = delete;
};

struct func
{
    int& i;
    func(int& i_) : i(i_) {}

    void operator()() {
        for (unsigned j = 0; j < 1000u; ++j) {
            // 对悬空引用的可能访问
            std::cout << "i = " << i << std::endl;
        }
    }
};

int main() {
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread t(my_func);
    {
        thread_guard g(t);
    }
    std::cout << "Thread had joined\n";
}
