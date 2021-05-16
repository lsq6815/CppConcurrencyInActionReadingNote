#include <thread>
#include <iostream>
#include <string>

class background_task {
private:
    std::string name;
public:
    // 一旦你实现了构造、复制构造，赋值运算符、移动构造、移动赋值运算符之一
    // 其他的默认实现就会失效。所以这里通过「= default」显式的使用默认构造
    background_task() = default;

    background_task(const background_task& bt) {
        name = bt.name;
        std::cout << "Copy constructor is called\n";
    }

    void operator()() const {
        std::cout << "Hello " << name 
                << " welcome to C++ Concurrency World\n";
    }

    void set_name(const std::string& name_) {
        name = name_;
    }
};

int main(void) {
    background_task f1, f2;
    f1.set_name("LSQ");
    f2.set_name("FWF");

    std::thread t1(f1);
    t1.join();

    std::thread t2(f2);
    t2.join();
}