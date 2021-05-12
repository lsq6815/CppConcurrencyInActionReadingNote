# 第 1 章 你好，C++ 并发世界

在 C++98 之后的 13 年后，C++11 终于将**语言级**的多线程纳入标准，C++ 程序员再也不用依赖系统级的线程支持（例如 `Pthreads` 和 `win32api`），可以编写可移植的多线程代码。那么什么是**并发（concurrency）**和**多线程（multi-threading）**？

## 开始入门

- 你好，C++ 世界

    ```cpp
    #include <iostream>

    int main(int argc, char const *argv[]) {
        std::cout << "Hello World\n";
        return 0;
    }
    ```

- 你好，C++ 并发世界

    ```cpp
    /// @file hello_concurrency_world.cpp
    #include <iostream>
    #include <thread>

    void hello() 
    {
        std::cout << "Hello, Concurrency World\n";
    }

    int main(int argc, char const *argv[]) 
    {
        std::thread t(hello);
        t.join();
        return 0;
    }
    ```

区别：

1. 增加了 `#include <thread>`。标准库头文件 `thread` 包含了管理线程的函数和类，而保护共享数据的函数和类在其他头文件声明。

2. 消息在 `void hello()` 中打印。每个线程都必须具有一个**初始函数（initial function）**，新的线程在其中执行。

    | 线程     |                     初始函数 |
    | :------- | ---------------------------: |
    | 初始线程 |                `main()` 函数 |
    | 其他线程 | `std::thread` 构造函数中指定 |

    是的，`main()` 函数是程序初始线程的初始函数。

3. 使用新线程输出「Hello, Concurrency World」。初始线程始于 `main()` 而新线程始于 `hello()`。在新的线程启动后，初始线程**继续**执行。如果不让它等待新线程结束，它就会自顾自的执行到 `main()` 结束，从而结束程序——有可能在新线程开始之前。这就是调用 `t.join()` 的原因，它使得调用线程（caller）等待被调用线程（callee）结束。
