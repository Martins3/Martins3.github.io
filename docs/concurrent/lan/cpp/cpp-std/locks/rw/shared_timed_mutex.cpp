// shared_timed_mutex 的基本用法
//
// shared_timed_mutex 和 shared_mutex 一样，都是“读者-写者锁”：
// 1. 多个读线程可以同时持有共享锁
// 2. 写线程需要独占锁，写入期间不能和其他读写并发
//
// 它和 shared_mutex 的主要区别是：额外提供了带超时的加锁接口，
// 比如 try_lock_for() / try_lock_until()。
// 不过这个示例里没有使用 timed API，而是演示：
// “给自己加独占锁、给另一个对象加共享锁” 的组合场景。
//
// 这个模式常见于拷贝赋值：
// 1. 需要读取 other 的内容，所以对 other 只要共享锁
// 2. 需要修改 *this，所以对自己必须拿独占锁
//
// https://en.cppreference.com/w/cpp/thread/shared_timed_mutex
#include <mutex>
#include <shared_mutex>
#include <iostream>
#include <thread>

class R {
	mutable std::shared_timed_mutex mut;
	/* data */
	int a;

    public:
	R &operator=(const R &other)
	{
		// 赋值时要修改当前对象，所以 *this 需要独占锁。
		std::unique_lock<std::shared_timed_mutex> lhs(mut,
							      std::defer_lock);
		// 这里只是读取 other，因此给 other 加共享锁即可。
		std::shared_lock<std::shared_timed_mutex> rhs(other.mut,
							      std::defer_lock);
		// defer_lock 表示先构造锁对象，但暂时不真正上锁。
		// 再交给 std::lock 一次性获取两个锁，避免手写加锁顺序带来的死锁风险。
		std::lock(lhs, rhs);
		/* assign data */
		printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
		return *this;
	}
};

int main()
{
	R l;
	R r;
	// 这里触发一次赋值，演示“左边独占锁 + 右边共享锁”的组合。
	l = r;
	sleep(1);
}
