# 第 1 章 你好，C++ 并发世界

- [第 1 章 你好，C++ 并发世界](#第-1-章-你好c-并发世界)
  - [什么是并发](#什么是并发)
    - [计算机系统的并发](#计算机系统的并发)
    - [并发的途径](#并发的途径)
      - [多进程并发](#多进程并发)
        - [Pros of multi-process](#pros-of-multi-process)
        - [Cons of multi-process](#cons-of-multi-process)
      - [多线程并发](#多线程并发)
        - [Pros of multi-thread](#pros-of-multi-thread)
        - [Cons of multi-thread](#cons-of-multi-thread)
  - [为什么使用并发](#为什么使用并发)
    - [为了划分关注点](#为了划分关注点)
    - [性能、性能还是性能](#性能性能还是性能)
  - [在 C++ 中使用并发和多线程](#在-c-中使用并发和多线程)
  - [开始入门](#开始入门)

在 C++98 之后的 13 年后，C++11 终于将**语言级**的多线程纳入标准，C++ 程序员再也不用依赖系统级的线程支持（例如 `Pthreads` 和 `win32api`），可以编写可移植的多线程代码。那么什么是**并发（concurrency）**和**多线程（multi-threading）**呢？

## 什么是并发

并发，在同一个时间段内两个或多个活动同时发生。

并行，在同一个时间点上两个或多个活动同时发生。

### 计算机系统的并发

对于早期只有一个处理器，处理器只有单个处理单元（CPU）或核心的计算机。这种计算机是串行的，但可以通过快速的**任务切换（task switching）**，来实现**并发**执行的**假象**。

```cpp
单核：A|B|A|B|A|B|...|A|B|A|B|...
```

随着时代的发展，具有多个处理器、单个处理器有多个核心（多核处理器）的计算机也越来越常见，前者多见于服务器和高性能计算，后者多见于 PC。无论是多处理器还是多核处理器的机器，都能**真正的**实现并行，我们称为**硬件并发（hardware concurrency）**。

理想状态下，每个核心分别允许一个任务，当然 CPU 1 交替运行 task A 和 task B，CPU 2 也交替运行 task A 和 task B 的情况也不是不可能的，就算如此多核处理器执行的**上下文切换（context switch）** 也比单核的少的多。

除了多处理器和多核处理器上的硬件并发，有的处理器可以在一个核心上执行多个线程。但是设备的**硬件线程（hardware threads）**是有限的。在硬件线程耗尽的情况下，任务切换还是会被启用。

### 并发的途径

#### 多进程并发

##### Pros of multi-process

- 丰富的进程间通信机制（IPC）
  
- 更安全的并发

- 远程进程调用

##### Cons of multi-process

- IPC 要么速度慢，要么机制复制，或者两者都有。

- 额外的系统开销

#### 多线程并发

##### Pros of multi-thread

- 轻松共享地址空间、内存

- 开销低

- C++ 没有对 IPC 的原生支持，只有对多线程的原生支持 -_-||

##### Cons of multi-thread

- 你自己负责保护数据！

## 为什么使用并发

### 为了划分关注点

- GUI：下载器一边下载文件，一边响应用户的操作；
 
- 后台任务：一边浏览歌单，一边后台播放音乐。

### 性能、性能还是性能

有 2 种使用并发优化性能的方式：

- 任务并行（task parallelism）：将一个过程分为并行的几个小过程（实际编写相当困难）

- 数据并行（data parallelism）：每个线程在不同的数据部分上执行同一个操作（比如多线程下载，最常见）

容易受这种并行影响性能的算法常被称为：易并行（embarrassingly parallel）、自然并行（naturally parallel）或便利并发（conveniently concurrent）。对于不易并发的算法，可以将算法换分为固定数量的并行任务，在线程间划分任务的技巧在第 8 章讨论。

## 在 C++ 中使用并发和多线程

虽然 C++98 不承认线程的存在，并且语言标准使用顺序抽象机的形式编写，但是这阻止不了编译器通过扩展实现对线程的支持，但是它们通常只能使用 POSIX C 和 Microsoft Windows API 这种平台相关的 API。

由于不满使用平台相关的 API 来处理多线程。C++ 程序员转而寻求类库对多线程的支持。像 MFC 这样的应用程序框架，以及 Boost 和 ACE 这样 C++ 通用类库，都实现了对平台相关的 API 的封装。但不同的库使用的方式都不同，特别是启动新线程。但是有一种习惯用法被许多 C++ 类库同时使用，就是带锁的**资源获取即初始化（RAII，Resource Acquisition Is Initialization）**

随着 C++11 标准的发布。C++ 不仅有了一个全新的线程感知内存模型，C++ 标准库也被扩展了，包含了用于管理线程（第 2 章）、保护共享数据（第 3 章）、线程间同步操作（第 4 章）以及低级原子操作（第 5 章）的各个类。

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

    void hello() {
        std::cout << "Hello, Concurrency World\n";
    }

    int main(int argc, char const *argv[]) {
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
