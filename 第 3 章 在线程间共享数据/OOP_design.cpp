#include <algorithm>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>

template <typename T> class Protected_List {
private:
  std::list<T> data;
  std::mutex lock;

  using mutex_guard = std::lock_guard<std::mutex>;

public:
  void add_to_list(T new_value) {
    mutex_guard guard(lock);
    data.push_back(new_value);
  }

  bool list_contains(T value_to_find) {
    mutex_guard guard(lock);
    return std::find(data.begin(), data.end(), value_to_find) != data.end();
  }

  void display() {
    mutex_guard guard(lock);
    std::for_each(data.begin(), data.end(),
                  [](T item) { std::cout << item << ", "; });
  }
};

int main() {
  Protected_List<int> lis;
  std::thread t1(
      [](Protected_List<int> &lis) {
        int count = 100;
        while (count--) {
          lis.add_to_list(count);
        }
      },
      std::ref(lis));

  std::thread t2(
      [](Protected_List<int> &lis) {
        int count = 100;
        while (count--) {
          if (lis.list_contains(count)) {
            lis.add_to_list(count);
          }
        }
      },
      std::ref(lis));

  t1.join();
  t2.join();
  lis.display();
}
