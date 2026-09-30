#include <coroutine>
#include <iostream>
#include <chrono>
#include <thread>

auto dbg = [](const char *s) { std::cout << "Function " << s << " called.\n"; };

struct Sleeper {
	bool await_ready() const noexcept
	{
		dbg(__PRETTY_FUNCTION__);
	}
	void await_suspend(std::coroutine_handle<> h) const
	{
		dbg(__PRETTY_FUNCTION__);
	}
	void await_resume() const noexcept
	{
		dbg(__PRETTY_FUNCTION__);
	}
	const std::chrono::duration<int, std::milli> length;
};

int main()
{
}
