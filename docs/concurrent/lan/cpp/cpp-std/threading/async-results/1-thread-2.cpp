// https://stackoverflow.com/questions/18143661/what-is-the-difference-between-packaged-task-and-async

#include <future>
#include <iostream>
#include <thread>

void packaged_task_test()
{
	std::packaged_task<int(int, int)> task([](int a, int b) {
		sleep(60);
		return a + b;
	});
	auto f = task.get_future();
	printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	task(2, 3);
	std::cout << f.get() << '\n';
}

// count down taking a second for each value:
int countdown(int from, int to)
{
	for (int i = from; i != to; --i) {
		std::cout << i << '\n';
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	std::cout << "Lift off!\n";
	return from - to;
}
void packaged_task_test2()
{
	std::packaged_task<int(int, int)> tsk(countdown);
	// set up packaged_task
	std::future<int> ret = tsk.get_future(); // get future
	// spawn thread to count down from 10 to 0
	std::thread th(std::move(tsk), 10, 0);
	// 理解了，原来还可以让 packaged_task 通过 thread 的方式，也可以通过
	int value = ret.get(); // wait for the task to finish and get result
	std::cout << "The countdown lasted for " << value << " seconds.\n";
	th.join();
}

void async_test()
{
	auto f = std::async(
		std::launch::async,
		[](int a, int b) {
			sleep(60);
			return a + b;
		},
		2, 3);
	std::cout << f.get() << '\n';
}

int main(int argc, char *argv[])
{
	/* packaged_task_test(); */
	packaged_task_test2();
	/* async_test(); */
	return 0;
}
