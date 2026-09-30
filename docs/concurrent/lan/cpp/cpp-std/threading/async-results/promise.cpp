#include <future>
#include <iostream>
#include <thread>

int main()
{
	std::promise<int> p;
	std::future<int> f = p.get_future();

	std::thread t([&p] {
		try {
			// code that may throw
			throw std::runtime_error("Example");
		} catch (...) {
			try {
				// store anything thrown in the promise
				p.set_exception(std::current_exception());
			} catch (...) {
			} // set_exception() may throw too
		}
	});

	try {
		std::cout << f.get();
	} catch (const std::exception &e) {
		std::cout << "Exception from the thread: " << e.what() << '\n';
	}
	t.join();
}
