#include <chrono>
#include <iostream>
#include <thread>

void f()
{
	std::cout << "hello world" << std::endl;
}

int main()
{
	std::thread t{ f }; // 启动会瞬间将 cpp 的线程拉起来
	std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	t.join(); // 等待新起的线程退出
}
