#include <cstdio>
#include <chrono>
#include <functional>
#include <thread>

// 案例 1:
// 1. lambda 的内部可以使用局部变量，这让函数内部的局部变量相当于变成 lambda 的参数，这非常好
void foo(int target) {
    std::vector<int> v = {3, 1, 4, 2};

    std::sort(v.begin(), v.end(),
        [target](int a, int b) {
            return abs(a - target) < abs(b - target);
        });
}


// 案例 2
struct Result {
	int bytes;
};

// 模拟 async read
void async_read(std::function<void(Result)> callback)
{
	// TODO 不可以用引用的方法来实现，原因需要继续思考下
	// std::thread([&callback]() {
	std::thread([callback]() {
		std::this_thread::sleep_for(std::chrono::seconds(1));

		Result result{ 1024 };
		callback(result);
	}).detach();
}

// 这是演示的核心，不同的 send_request 可以等价于
//
// struct Callback {
//     int request_id;
//
//     void operator()(Result result) const {
//         printf("%d\n", request_id);
//     }
// };
//
// void send_request(int request_id) {
//     Callback callback{request_id};
//     async_read(callback);
// }
void send_request(int request_id)
{
	async_read([request_id](Result result) {
		printf("request %d completed, bytes=%d\n", request_id,
		       result.bytes);
	});
}

void test_async_callback(){
	send_request(100);
	send_request(200);

	printf("send_request returned\n");

	std::this_thread::sleep_for(std::chrono::seconds(2));
}

// 案例 3 : docs/concurrent/lan/cpp/cpp.md 中分析 c++ 的 thread 来使用
int main()
{
}
