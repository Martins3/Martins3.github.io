// https://itnext.io/c-20-coroutines-complete-guide-7c3fc08db89d
#include <iostream>
#include <coroutine>

// The caller-level type
struct Task {
	// The coroutine level type
	struct promise_type {
		Task get_return_object()
		{
			std::cout <<  "get_return_object" << std::endl;
			return {};
		}
		std::suspend_never initial_suspend()
		{
			std::cout << "initial_suspend" << std::endl;
			return {};
		}
		std::suspend_never final_suspend() noexcept
		{
			std::cout << "final_suspend" << std::endl;
			return {};
		}
		void return_void()
		{
			std::cout << "return_void" << std::endl;
		}
		void unhandled_exception()
		{
			std::cout << "unhandled_exception" << std::endl;
		}
	};
};

Task myCoroutine()
{
	std::cout << "hi" << std::endl;
	co_return; // make it a coroutine
}

int main()
{
	myCoroutine();
}
