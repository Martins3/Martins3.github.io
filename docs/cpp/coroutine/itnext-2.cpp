// https://itnext.io/c-20-coroutines-complete-guide-7c3fc08db89d

#include <coroutine>
#include <iostream>

auto dbg = [](const char *s) { std::cout << "Function " << s << " called.\n"; };

// The caller-level type
struct Generator {
	// The coroutine level type
	struct promise_type {
		using Handle = std::coroutine_handle<promise_type>;

		Generator get_return_object()
		{
			dbg(__PRETTY_FUNCTION__);
			return Generator{ Handle::from_promise(*this) };
		}
		std::suspend_always initial_suspend()
		{
			dbg(__PRETTY_FUNCTION__);
			return {};
		}
		std::suspend_always final_suspend() noexcept
		{
			dbg(__PRETTY_FUNCTION__);
			return {};
		}
		std::suspend_always yield_value(int value)
		{
			dbg(__PRETTY_FUNCTION__);
			current_value = value;
			return {};
		}
		void unhandled_exception()
		{
			dbg(__PRETTY_FUNCTION__);
		}
		int current_value;
	};

	explicit Generator(promise_type::Handle coro)
		: coro_(coro)
	{
		dbg(__PRETTY_FUNCTION__);
	}
	// Make move-only
	Generator(const Generator &) = delete;
	Generator &operator=(const Generator &) = delete;
	Generator(Generator &&t) noexcept : coro_(t.coro_)
	{
		dbg(__PRETTY_FUNCTION__);
		t.coro_ = {};
	}
	int get_next()
	{
		dbg(__PRETTY_FUNCTION__);
		coro_.resume();
		return coro_.promise().current_value;
	}

    private:
	promise_type::Handle coro_;
};

Generator myCoroutine()
{
	int x = 0;
	while (true) {
		co_yield x++;
	}
}

int main()
{
	auto c = myCoroutine();
	int x = 0;
	while ((x = c.get_next()) < 10) {
		std::cout << x << "\n";
	}
}
