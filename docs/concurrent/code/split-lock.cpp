#include <bits/stdc++.h>
// #include "../dbg.hpp"
using namespace std;

auto niter = 100000;
void non_split() {

  std::atomic<uint32_t> v;
  auto start = std::chrono::system_clock::now();
  for (std::size_t i = 0; i < niter; ++i) {
    v.fetch_add(1, std::memory_order_acquire);
  }
  auto stop = std::chrono::system_clock::now();
  std::cout << (stop - start).count() / niter << std::endl;
}

void split() {
  char buf[128] = {};
  auto *v = reinterpret_cast<std::atomic<uint32_t> *>((uint64_t)buf | 61);
  auto start = std::chrono::system_clock::now();
  for (std::size_t i = 0; i < niter; ++i) {
    v->fetch_add(1, std::memory_order_acquire);
  }
  auto stop = std::chrono::system_clock::now();
  std::cout << (stop - start).count() / niter << std::endl;
}

int main(int argc, char *argv[]) {
  non_split();
  split();
  return 0;
}
