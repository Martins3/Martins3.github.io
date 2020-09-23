# conditional variable

```c
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

// 1. 代码总是一行一行的按照规则执行
// 2. wait 释放锁，被 notify 之后申请锁
// 3. while (!ready) 应该是 : 在处理队列这种情况，多个线程由于缓冲区耗尽而等待，当所有人唤醒之后，A 获取锁，消耗资源，释放锁，B 获取锁，但是必须检查一下资源才可以。
void print_id(int id) {
  std::unique_lock<std::mutex> lck(mtx); 
  while (!ready) {
    std::cout << std::this_thread::get_id() << " before wait" << std::endl;
    cv.wait(lck); // 释放锁
    std::cout << std::this_thread::get_id() << " after wait" << std::endl;
  }

  ready --;

  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::cout << "thread " << id << '\n';
}

void go() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::unique_lock<std::mutex> lck(mtx);
    ready = 2;
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
```


## 继承

#### 权限和继承

1. 下面的代码没有错误，也不科学，@todo 找到
```cpp
class A{
public:
  virtual void gg(){
    cout << "A" << endl;
  }
};

class B : public A{
private:
  void gg(){
    cout << "B" << endl;
  }
};

void bb(A * a){
  a->gg();
}

int main() {
  B * b = new B;
  bb(b);
  return 0; 
}
```

