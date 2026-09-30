// https://theshoemaker.de/posts/yet-another-cpp-coroutine-tutorial
// TODO 戛然而止的感觉
#include <coroutine>

class BasicCoroutine {
    public:
	struct Promise {
		BasicCoroutine get_return_object()
		{
			return BasicCoroutine{};
		}

		void unhandled_exception() noexcept
		{
		}

		void return_void() noexcept
		{
		}

		std::suspend_never initial_suspend() noexcept
		{
			return {};
		}
		std::suspend_never final_suspend() noexcept
		{
			return {};
		}
	};
	using promise_type = Promise;
};

BasicCoroutine coro()
{
	co_return;
}

int main()
{
	coro();
}
