# 第 2 章 线程管理

- [第 2 章 线程管理](#第-2-章-线程管理)
  - [内容提要](#内容提要)
  - [线程基本管理](#线程基本管理)
    - [启动线程](#启动线程)
      - [使用函数指针作为参数](#使用函数指针作为参数)
      - [使用函数对象作为参数](#使用函数对象作为参数)
      - [使用 lambda 表达式作为参数](#使用-lambda-表达式作为参数)
    - [决定线程的运行方式](#决定线程的运行方式)
    - [等待线程完成](#等待线程完成)
    - [在异常的环境下等待线程](#在异常的环境下等待线程)
    - [在后台运行线程](#在后台运行线程)
  - [传递参数给线程函数](#传递参数给线程函数)
    - [小心隐式转换](#小心隐式转换)
    - [引用外部变量](#引用外部变量)
    - [处理只能被移动的参数](#处理只能被移动的参数)
  - [转移线程的所有权](#转移线程的所有权)
  - [在运行时选择线程的数量](#在运行时选择线程的数量)
  - [表示线程](#表示线程)

## 内容提要

- 启动线程，以及各种让代码在新线程上允许的方法

- 等待线程完成并让它自然运行

- 唯一地表示线程

## 线程基本管理

每个 C++ 程序都拥有至少一个线程，它是由 C++ 在运行时启动的，该线程运行着 `main()` 函数。

### 启动线程

无论线程从哪里启动或是要做什么，使用 C++ 线程库来开始一个线程总归要构造一个 `std::thread` 对象。

#### 使用函数指针作为参数

你可以使用**函数指针**作为参数。

```cpp
void do_some_work(); // 注意：这不是定义类的实例，而是声明函数，这是 C++ 一个臭名昭著的缺点。
std::thread my_thread(do_some_work);
```

#### 使用函数对象作为参数

与很多 C++ 标准库类似，`std::thread` 接受一个 **可调用（callable）** 的类型作为参数。也就是有 `operator()` 的类实例可以被传递给 `std::thread` 的构造函数，即仿函数或**函数对象（function object）**。

```cpp
class background_task 
{
public:
    void operator()() const {
        do_something();
        do_something_else();
    }
};
background_task f;
std::thread my_thread(f);
```

在这种情况下，类实例被**复制（copied）**到属于新创建的执行线程的存储器中。所以确保副本与原版有着等效的行为是重要的，比如常见的**浅复制**问题。

```cpp
/// @file object_as_param.cpp
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
/**
 * output:
 * Copy constructor is called
 * Hello LSQ welcome to C++ Concurrency World
 * Copy constructor is called
 * Hello FWF welcome to C++ Concurrency World
 */
```

代码见 [object_as_param.cpp](./object_as_param.cpp) 。

除了复制对象的问题，还有一个问题就是「C++ 的最棘手的解析」。如果你传递一个临时的匿名变量，那么其语法可能与函数声明一致。例如：

```cpp
std::thread my_thread(background_task());
```

上面的代码声明一个函数 `my_thread`，它接受单个参数，这个参数的类型是函数指针，指向一个不接受参数，返回 `background_task` 类型的对象的函数。回到函数 `my_thread`， 它返回 `std::thread` 类型的对象。

如何解决呢？在 C++11 之前，你可以添加额外的括号来阻止编译器解析为函数声明：

```cpp
std::thread my_thread((background_task()));
```

在 C++11 之后，统一初始化语法可以解决问题：

```cpp
// 这三种写法都可以编译通过
std::thread my_thread(background_task{});
std::thread my_thread{background_task()};
std::thread my_thread{background_task{}};
```

#### 使用 lambda 表达式作为参数

**lambda 表达式（lambda expression）**是 C++11 中的新功能，其基本功能是编写一个局部匿名的函数，并可以捕捉一些局部变量：

```cpp
std::thread my_thread([]() {
    do_something();
    do_something_else();
});
```

### 决定线程的运行方式

一旦开始了线程，你需要**显式**地决定是要等待它完成（`std::thread::join()`）还是让它自行运行（`std::thread::detach()`）。如果你在 `std::thread` 对象被析构前未作出决定，那么 `std::thread` 的析构函数会调用 `std::terminate()` 终止你的程序。

> 需要注意的是，你只需要在 `std::thread` 对象被析构前做出这个决定即可，线程本身可能在你结合（`join()`）或分离（`detach()`）之前就结束了，而且如果你分离它，那么该线程可能在 `std::thread` 对象析构很久后都还在运行。

如果你不等待线程完成，那么你需要确保通过该线程访问的数据是有效的。一个可能的情况是，当线程持有局部变量的指针或引用，当当函数退出时线程还未完成：

```cpp
/// @file local_var_ref.cpp
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
```

代码见 [local_var_ref.cpp](local_var_ref.cpp) 。

一种常见的处理方式是使线程函数自包含，并且把数据复制（copy）到线程中而不是共享数据。

### 等待线程完成

`std::thread::join()`。其实没什么用，因为这跟直接调用函数没啥区别，又简单又暴力，做不到细粒度的控制。

调用 `join()` 的行为会导致清理所有与线程有关的存储器，这样 `std::thread` 对象将不再与任何线程相关联。也就是说你对一个给定的线程只能调用一次 `join()`，之后 `std::thread` 对象不再可连接，并且 `joinable()` 将返回 `false`。

```cpp
/// @file after_join_thread_is_unjoinable.cpp
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
```

代码见 [after_join_thread_is_unjoinable.cpp](after_join_thread_is_unjoinable.cpp) 。

### 在异常的环境下等待线程

如果你决定分离线程，通常在线程启动后就可以立即调用 `detach()`。但如果打算等待线程，就需要仔细的选择在代码的哪个位置调用 `join()`。一个问题是：如果在线程开始之后但又在调用 `join()` 之前引发了异常，那么对 `join()` 的调用就容易被跳过。

为了解决上面的问题，你需要在非异常的状态下调用 `join()`，在异常状态下也调用一次 `join()`，例如：

```cpp
struct func;

void f() {
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread t(my_thread);

    try {
        do_something_in_current_thread();
    }
    catch (...) {
        t.join();
        throw;
    }

    t.join();
}
```

很啰嗦对吧，而且 `try/catch` 块会弄乱你的作用域，所以这不是一个理想的方案。Java 对此的解决方案是 `finally` 块，确保无论异常是否发生都会执行其中的语句。但 C++ 偏好另一种方法（毕竟 C++ 也没有 `finally` 块），那就是**标准的资源获取即初始化（RAII）**惯用语法，并提供一个类，在它的析构函数中执行 `join()` 。例如：

```cpp
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

struct func;

void func() {
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread t(my_func);
    thread_guard g(t);

    do_something_in_current_thread();
}
```

代码见 [thread_guard.cpp](thread_guard.cpp) 。

在当前线程执行到 `func()` 末尾时，局部对象会安装构造函数调用的**逆序**进行析构。因此 `thread_guard` 对象 `g` 首先被析构，并在析构函数中结合线程。即使因为 `do_something_un_current_thread()` 引发异常而造成退出也一样。

复制构造和赋值运算符被标记为 `=delete` 以确保编译器不会自动地提供它们。赋值或复制一个怎样的对象是危险的。有些 C++ 大佬甚至建议编译器不要自动提供复制构造和赋值运算符，或默认加上 `=delete`。

> 注意：在结合线程前一定要测试它是否可结合。

### 在后台运行线程

在 `std::thread` 对象上调用 `detach()` 回报线程丢在后台，没有与之通信的方法，也可能等待他完成，更不可能获取一个引用它的 `std::thread` 对象。分离的线程在后台运行时，所有权和控制权转交给 C++ Runtime，以确保在线程退出后正确回收线程相关资源。

线程分离后，`std::thread` 对象不再与实际的线程相关联，也不可能被加入。

```cpp
std::thread t(do_background_work);
t.detach();
assert(!t.joinable());
```

与 `join()` 类似，线程也只能分离一次。

```cpp
if (t.joinable()) 
    t.detach();
```

什么时候使用 `detach()` 呢？考虑一个文本编辑器，它对每个文件分别打开一个顶级窗口，然后运行同样的代码，因为文件互不关联，所以可以使用 `detach()`。

```cpp
void edit_document(const std::string& filename) {
    open_document_and_display_gui(filename);
    while(!done_editing()) {
        user_command cmd = get_user_input();
        if (cmd.type == open_new_document) {
            const std::string new_doc = get_filename_from_user();
            std::thread t(edit_document, new_doc);
            t.detach();
        }
        else {
            process_user_input(cmd);
        }
    }
}
```

如你所见，除了使用带有成员数据的函数对象来传递参数外，`std::thread` 提供了更简单的方法：

```cpp
std::thread t(func, p1, [p2, [p3, ...]])
```

## 传递参数给线程函数

### 小心隐式转换

如上所示，传递参数给函数指针或函数对象只是简单地将参数传入 `std::thread` 的构造函数。重要的是，参数会以默认的方式被**复制到（copied）**线程的内部存储空间，**然后**新线程可以访问它们，**即便**函数中的相应参数期待**引用**。例如：

```cpp
void f(int i, const std::string& s);
std::thread t(f, 3, "Hello");
```

这里创建一个新的与 `t` 关联的执行线程，称为 `f(3, "hello")`。注意即使 `f` 接受一个 `std::string` 作为第二个参数，在 C++ 中字符串字面量是 `const char[N]` 类型（`N` 是字符串的大小）的 null-terminate 的字符串数组，而不是 `std::string` 的常量（隔壁 Java、Python 等一大串语言就没这问题），而 C++ 不提供对数组的传参，所以被**复制（copied）**到新线程上下文的其实是 `const char *` 类型的指针，**之后**新线程在访问它的时候才会调用 `std::string()` 的构造函数进行类型转换。尤其重要的是当我们提供的参数是一个自动变量的指针，如下所示：

```cpp
void f(int i, const std::string& s);

void oops(int some_param) {
    char buffer[1024];
    sprintf(buffer, "%i", some_param);
    std::thread t(f, 3, buffer);
    t.detach();
}
```

这里，`buffer` 的指针被复制给新线程，如果 `oops()` 在新线程把 `buffer` 的指针转换为 `std::string` 对象之前就退出了，就会导致恐怖的未定义行为**（UB，undefined behavior）**。

解决方法是在 `buffer` 传递给 `std::thread` 的构造函数前就转换它为 `std::string` 的对象：

```cpp
std::thread t(f, 3, std::string(buffer));
```

这种问题发生的原因是：程序员依赖从 `buffer` 到 `std::string` 对象的**隐式转换**，但 `std::thread` 的构造函数原样复制了它接受的值，即 `const char*`，而不是转换为 `std::string` 再复制。

### 引用外部变量

也有另一种问题，对象被正确复制但你要的是引用。这可能发生在当线程正在更新一个通过引用传递来的数据结构时，例如：

```cpp
void update_data_for_widget(widget_id w, widget_data& data);

void oops_again(widget_id w) {
    widget_data data;
    std::thread t(update_data_for_widget, w, data);
    display_status();
    t.join();
    process_widget_data(data);
}
```

尽管 `update_data_for_widget` 希望**引用**第二个参数，但 `std::thread` 的构造函数并不知道，它**无视**线程函数所期望的参数类型，并且**盲目地复制**程序员提供的参数。当 `std::thread` 调用 `update_data_for_widget` 时，它将复制 `data` 的副本供 `update_data_for_widget` 引用。于是，当线程完成后，所有改动都会消失，外部的 `data` 没有任何影响。

对于熟悉 `std::bind` 的人来说，解决方法是显而易见的，使用 `std::ref` 来包装确实需要被引用的参数：

```cpp
std::thread t(update_date_for_widget, w, std::ref(data));
```

> 事实上，`std::ref` 的功能是提供一个包装类供 `std::thread` 复制过去，并不是语言级的功能。想想为什么 C 语言只有值传参却可以使用指针达成引用的效果。因为被复制的指针包含同样的值，指向变量的地址。

如果你熟悉 `std::bind` 那么参数传递语义就不足为奇，因为 `std::thread` 的构造函数和 `std::bind` 的操作都是依据相同的机制定义的。这意味着，你可以传递一个成员函数的指针作为函数，前提是提供一个合适的对象指针作为第一个参数：

```cpp
class X {
public:
    void do_lengthy_work();
};

X my_x;
std::thread t(&X::do_lengthy_work, &my_x);
```

### 处理只能被移动的参数

提供参数的另一个有趣的场景是，这里的参数不能被复制只能被**移动（moved）**：一个对象内保存的数据被转移到另一个对象，使得原来的对象变为「空壳」。这种类型的典型例子是 `std::unique_ptr`。**移动构造函数（move constructor）**和**移动赋值运算符（move assignment operator）**允许一个对象的所有权在 `std::unique_ptr` 实例之间进行移动。这种转移给源对象留下一个 `nullptr` 指针。这种值的移动使得该类型的对象可以作为函数的参数或作为函数的返回值：

```cpp
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args ) {
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}
```

> 这是 Herb 给的 C++11 的 `make_unique` [实现](https://herbsutter.com/gotw/_102/)。（C++11 有 `make_shared` 和 `make_weak`，但 `make_unique` 直到 C++14 才被补上，Herb 说他们忘了）

在源对象是临时的场合，这种移动是自动的（编译器视其为 `rvalue` 并优化），但在源是 `lvalue` 的情况下，这种转移必须通过调用 `std::move()` 来请求。例如：

```cpp
void process_bug_object(std::unique_ptr<big_object>);

std::unique_ptr<big_object> p(new big_object);
p->prepare_data(42);
std::thread t(process_big_object, std::move(p));
```

这种可移动（moveable），**不**可复制（copyable）的所有权语义除了 `std::unique_ptr` 外，在 C++ 标准库内也有其他的类也拥有，比如 `std::thread`。

## 转移线程的所有权

假设你要编写一个函数，它创建一个在后台运行的线程，但是向 caller 回传新线程的所有权，而不是等待它完成。又或者你要反过来做，创建一个线程，将所有权传递给要等待他完成的 callee。任何一种情况下，你都需要处理线程的所有权。

正如上文所述，C++标准库中有许多拥有资源的类型，如 `std::ifstream` 和 `std::unique_ptr` 以**可移动（moveable），不可复制（copyable）**的，其中就包括 `std::thread`。这意味着一个特定线程的所有权可以在 `std::thread` 实例间移动，例如：

```cpp
void func1();
void func2();

std::thread t1(func1);
std::thread t2 = std::move(t1); // 对于 lvalue 需要显式地调用 std::move()

t1 = std::thread(func2);        // 对于 rvalue 移动是自动和隐式的 

std::thread t3;                 // 默认构造出的 std::thread 不具有资源（即线程）
t3 = std::move(t2);             
t1 = std::move(t3);             // 会导致程序终止！
```

最后一行，因为 `t1` 已经有关联的线程（`func2`），所以会调用 `std::terminate()` 来终止程序。你**不能**通过向管理一个线程的 `std::thread` 对象赋一个新值来「舍弃」一个线程。

`std::thread` 能够移动意味着所有权可以很容易的从一个函数中被转移出，例如：

```cpp
std::thread f() {
    void func1();
    return std::thread(func2);
}

std::thread g() {
    void func2(int);
    std::thread t(func2, 42);
    return t;
}
```

同样的，如果要把所有权转移到函数中，它只能以值的形式接受 `std::thread` 的实例作为其中一个参数。例如：

```cpp
void f(std::thread t);

void g() {
    void func();
    f(std::thread(func));
    std::thread t(func);
    f(std::move(t));
}
```

既然 `std::thread` 可以移动，我们可以升级我们的 `thread_guard` 类（在 [在异常的环境下等待线程](#在异常的环境下等待线程) 这一章），使得他可以唯一获得对 `std::thread` 的所有权，我们称之为 `scoped_thread`。

```cpp
class scoped_thread {
private:
    std::thread t;
public:
    explicit scoped_thread(std::thread t_) : t(std::move(t_)) {
        if (!t.joinable()) 
            throw std::logic_error("No thread");
    }

    ~scope_thread() {
        t.join();
    }

    scoped_thread(const scoped_thread &) = delete;
    scoped_thread& operator=(const scoped_thread &) = delete;
};

struct func;

void f() {
    int local_var{};
    scoped_thread t(std::thread(func(local_var)));
    do_something_in_current_thread();
}
```

`std::thread` 对移动的支持同样考虑了 `std::thread` 对象的容器，如果那些容器是移动感知的（如更新后的 `std::thread`），你可以做到：

```cpp
void do_work(unsigned id);

void f() {
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < 20; ++i) {
        threads.push_back(std::thread(do_work, i));
    }
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
}
```

如果线程是被用来细分某种算法的工作，这往往正是所需的。在返回调用者之前，所有线程必须全部完成。
 
将 `std::thread` 对象放到 `std::vector` 中是迈向线程自动管理的一步。

## 在运行时选择线程的数量

`std::thread::hardware_currency()` 返回一个对于给定程序执行时能够真正并发运行的线程数量的提示，如果该信息不可用则函数可能会返回 0。

下面展示 `std::accumulate` 的一个简单的并行版本实现。它在线程之间划分所做的工作，使得每个线程都具有最小数目的元素以避免过多线程的开销。请注意，该实现假定所有操作都不引发异常 ，对这种算法的异常处理将在第 8 章讨论。

```cpp
template <typename Iterator, typename T>
struct accumulate_block
{
    void operator()(Iterator first, Iterator last, T& result) {
        result = std::accumulate(first, last, result);
    }
};

template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
    using ul_t = unsigned long;
    using cul_t = const unsigned long;

    cul_t length = std::distance(first, last);

    if (!length) 
        return init; // (1)

    cul_t min_per_thread = 25;
    cul_t max_threads = (length + min_per_thread - 1) / min_per_thread; // (2)

    cul_t hardware_threads = std::thread::hardware_concurrency();

    cul_t num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads); // (3)
    
    cul_t block_size = length / num_threads; // (4)

    std::vector<T> results(num_threads); // T 要存在默认构造函数
    std::vector<std::thread> threads(num_threads - 1); // (5)

    Iterator block_start = first;
    for (ul_t i = 0; i < (num_threads - 1); ++i) {
        Iterator block_end = block_start;
        std::advance(block_end, block_size); // (6)
        threads[i] = std::thread(  // (7)
            accumulate_block<Iterator, T>(),
            // 还记的吗，std::thread 只会盲目地复制参数，为了引用数据要使用 std::ref
            block_start, block_end, std::ref(results[i])); 
        block_start = block_end; // (8)
    }

    accumulate_block<Iterator, T>() (
        block_start, last, results[num_threads - 1]); // (9)
    std::for_each(threads.begin(), threads.end(),
        std::mem_fn(std::thread::join));  // (10)

    return std::accumulate(results.begin(), results.end(), init); // (11)
}
```

代码见 [parallel_accumulate](parallel_accumulate) 。

虽然代码很长，但它实际上很直观。如果输入范围为空 `(1)`，只返回初始值 `init`。否则，此时范围内至少有一个元素，于是用要处理的元素数量除以最小的块的大小 `min_per_thread`，以获取线程的最大数量 `max_threads`。这是为了避免当范围内只有 5 个值的时候，在一个 32 核的机器上创建 32 个线程。

要实际运行的线程是计算出的线程最大数和硬件线程数 `(3)` 的较小值 `std::min()`。你不会想要运行比硬件所能支持线程数还多的线程（**超额订阅，oversubscription**），因为上下文切换将意味着更多的线程会降低性能。如果 `std::thread::hardware_concurrency()` 返回 0，你只需要简单地替换上你所选择的数量，这里选择了 2 （`hardware_threads != 0 ? hardware_threads : 2`），运行太多线程在单核机器上只会减低速度，当你也不想错过并发的机会（`std::thread::hardware_concurrency()` 返回 0 只意味着信息不可用）。

每个待线程处理的长度是范围的长度除以线程的数量 `(4)`。对于不能整除的情况后面会处理。为了保存线程运算的结果，创建 `std::vector<T>`，同时创建 `std::vector<std::thread>`，注意你启动的线程比 `num_threads` 少 1（`threads(num_threads - 1)`）。因为主线程已经存在。

启动线程是一个简单的循环：递进 `block_end` 到当前块的结尾 `(6)`，并启动一个线程来累计次块的结果 `(7)`。下一个块的开始就是当前块的结束

> STL 的算法和容器接受的范围都是左闭右开的 `[first, last)`。

当你启动了所有的线程后，这个线程就可以处理最后的块 `(9)`。这就是你同时处理 `main` 线程负责的块和因未被整除而遗留的数据的地方。你只需要知道最后的元素的后一个位置是 `last` 即可。一旦完成对最后一个块的累计，使用 `std::for_each` 确保每个其他的线程都已经完成 `(10)`。最后通过 `std::accumulate` 将结果累加起来 `(11)`。

在你离开这个例子前，值得指出的是在类型 `T` 的加法运算符不满足结合律的地方（如 `float` 和 `double`），`parallel_accumulate` 的结果可能更 `std::accumulate` 有出入，这是因为并行算法将元素分组求和导致的。

![a_1 + a_2 + \ldots a_n \neq a_1 + (a_2 + a_3 + \ldots + a_n)](https://latex.codecogs.com/svg.image?a_1%20&plus;%20a_2%20&plus;%20%5Cldots%20a_n%20%5Cneq%20a_1%20&plus;%20(a_2%20&plus;%20a_3%20&plus;%20%5Cldots%20&plus;%20a_n))

> 是的没错，`float` 和 `double` 等浮点数由于设计上的问题，是不满足加法和乘法的结合律的。

除此之外，对迭代器的要求要更严格一些，它们必须至少是**前向迭代器（forward iterator）**，然而 `std::accumulate` 可以和单通**输入迭代器（Input iterator）**一起工作，同时 `T` 必须是**可默认构造的（default constructible）**以使得你可以创建 `results` 向量（`std::vector<T> results(num_threads)`）。

还有一件事，因为你不能直接从线程中返回值，你必须将 `results` 向量的分量引用给线程`std::ref(results[i])`，使用 `future` 从线程返回结果的方法将在第 4 章讨论。

这里线程所需的运行信息在启动时就被调用，包括其结果的存储位置。实际情况并非总是如此，有时我们总是要标识一个线程，通过 `std::thread` 的构造函数传入一个标识符是可以的，但我们希望对线程有全局的唯一表示，幸运的是 C++ 线程库提供预见了这个问题。

## 表示线程

线程标识符是 `std::thread::id` 类型的，有两种获取方式。

1. 对与线程关联的 `std::thread` 对象调用 `get_id()` 获得。如果 `std::thread` 未与线程关联，`get_id()` 返回默认构造的 `std::thread::id` 对象，表示「没有线程」。

2. 当前线程的标识符标识可以通过调用 `std::this_thread::get_id()` 获得，这也是定义在 `<thread>` 头文件中的。

`std::thread::id` 类型的对象可自由地复制和比较，不然就没有标识符的意义了。

- 如果两个 `std::thread::id` 类型的对象相等：
  - 同一个线程

  - 两者都是「没有线程」的值

- 如果两个 `std::thread::id` 类型的对象不相等：

  - 它们代表不一样的线程
  
  - 一个代表线程，一个具有「没有线程」的值

`std::thread::id` 类型提供了一套完整的比较运算符，为 `std::thread::id` 的所有不相等的值提供了总排序，所以它们表现就像你直觉上认为的那样：

![a < b\;\land\;b<c, \implies a < c](https://latex.codecogs.com/svg.image?a%20%3C%20b%5C;%5Cland%5C;b%3Cc,%20%5Cimplies%20a%20%3C%20c%20%20%20)

之类的性质。所以 `std::thread::id` 类型可以作为关系型容器的键、被排序、使用符合直觉的方式进行比较。标准库还提供了 `std::hash<std::thread::id>`，使得 `std::thread::id` 类型的值可以在新的无序关系容器中作为键。

`std::thread::id` 的一个常见用途是检查一个线程是否需要执行某些操作。例如线程像在 `parallel_accumulate` 中那样被用来分配工作，启动了其他线程的初始线程需要做的工作可能和其他算法不同。在这种情况下，它可以在启动其他线程前存储 `std::this_thread::get_id()` 的结构，然后算法的核心部分（对所有的线程都是公共的）可以对照所存储的值来检查自己的线程 ID。

```cpp
std::thread::id master_thread;
void some_core_part_of_algorithm() {
    if (std::this_thread::get_id() == master_thread) {
        do_master_thread_work();
    }
    do_common_work();
}
```

> 有没有想到 `fork()`？
