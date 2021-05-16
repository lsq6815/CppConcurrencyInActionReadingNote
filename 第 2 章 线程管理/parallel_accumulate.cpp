#include <numeric>
#include <iostream>
#include <algorithm>
#include <functional>
#include <chrono>
#include <thread>
#include <cassert>
#include <vector>

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
        return init;

    cul_t min_per_thread = 25;
    cul_t max_threads = (length + min_per_thread - 1) / min_per_thread;

    cul_t hardware_threads = std::thread::hardware_concurrency();

    cul_t num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads); 
    
    cul_t block_size = length / num_threads;

    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads - 1);

    Iterator block_start = first;
    for (ul_t i = 0; i < (num_threads - 1); ++i) {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        threads[i] = std::thread(
            accumulate_block<Iterator, T>(),
            block_start, block_end, std::ref(results[i]));
        block_start = block_end;
    }

    accumulate_block<Iterator, T>() (
        block_start, last, results[num_threads - 1]);
    std::for_each(threads.begin(), threads.end(),
        std::mem_fn(std::thread::join));

    return std::accumulate(results.begin(), results.end(), init);
}

template <class TimeT = std::chrono::milliseconds,
          class ClockT = std::chrono::steady_clock>
class Timer
{
    using timep_t = typename ClockT::time_point;
    timep_t _start = ClockT::now(), _end = {};

public:
    void tick() { 
        _end = timep_t{}; 
        _start = ClockT::now(); 
    }
    
    void tock() { _end = ClockT::now(); }
    
    template <class TT = TimeT> 
    TT duration() const {
        assert(_end != timep_t{});
        return std::chrono::duration_cast<TT>(_end - _start); 
    }
};

int main() {
    std::vector<int> v(static_cast<int>(1e8));
    std::fill(v.begin(), v.end(), 1);
    int result{};
    Timer clock;

    clock.tick();
    result = parallel_accumulate(v.begin(), v.end(), 0);
    clock.tock();

    std::cout << result << std::endl;
    std::cout << "Runtime = " << clock.duration().count() << " ms\n";

    clock.tick();
    result = std::accumulate(v.begin(), v.end(), 0);
    clock.tock();

    std::cout << result << std::endl;
    std::cout << "Runtime = " << clock.duration().count() << " ms\n";
}
/**
 * Outputs:
 * 100000000
 * Runtime = 181 ms
 * 100000000
 * Runtime = 803 ms 
 */