#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char *argv[]) {
  struct A {
    int a[100];
  };

  struct B {
    int x, y;
  };

  assert(!std::atomic<A>{}.is_lock_free());
  assert(std::atomic<B>{}.is_lock_free());
  assert(std::atomic<int>{}.is_always_lock_free);
  /* assert(std::atomic<A>{}.is_always_lock_free); */
  assert(std::atomic<B>{}.is_always_lock_free);

  std::atomic_flag x = ATOMIC_FLAG_INIT;
  x.clear(std::memory_order_release); // 将状态设为 false
  // 不能为读操作语义：memory_order_consume、memory_order_acquire、memory_order_acq_rel

  bool y = x.test_and_set(); // 将状态设为 true 且返回之前的值

  std::cout << "perfect" << std::endl;
  return 0;
}
