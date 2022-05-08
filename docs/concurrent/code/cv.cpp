// condition_variable example
#include <chrono>             // std::chrono::seconds
#include <condition_variable> // std::condition_variable
#include <iostream>           // std::cout
#include <iostream>           // std::cout, std::endl
#include <mutex>              // std::mutex, std::unique_lock
#include <thread>             // std::thread

std::mutex mtx;
std::condition_variable cv;
int ready = 0;

// 条件变量实际上使用了 unique_lock 来实现互斥
void print_id(int id) {
  std::unique_lock<std::mutex> lck(mtx);
  while (!ready) {
    std::cout << id << " before wait" << std::endl;
    cv.wait(lck); // 释放锁
    std::cout << id << " after wait" << std::endl;
  }

  ready--;

  // 当这个进程结束之后，其他的 thread 才可以从 wait 上离开
  // 由于对于 ready 的判断都是在 unique_lock 的保护下的
  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::cout << "thread " << id << '\n';
}

void go() {
  std::this_thread::sleep_for(std::chrono::seconds(3));
  std::unique_lock<std::mutex> lck(mtx);
  ready = 4;
  cv.notify_all();
}

int main() {
  std::thread threads[10];
  // spawn 10 threads:
  for (int i = 0; i < 10; ++i)
    threads[i] = std::thread(print_id, i);
  std::cout << "10 threads ready to race...\n";
  go(); // go!
  for (auto &th : threads)
    th.join();
  return 0;
}
