#include <iostream>
#include <iomanip>
#include <thread>

int main() {
    std::thread t{[](){
        std::cout << "thread running\n";
    }};

    t.join();
    std::cout << std::boolalpha << t.joinable() << std::endl;
}