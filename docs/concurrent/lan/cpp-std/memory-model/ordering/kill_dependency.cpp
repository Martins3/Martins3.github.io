#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

// 这个文件想解释的不是“kill_dependency 会让结果变掉”。
// 它返回的值和原来一样, 真正变化的是:
// “这个值是否还继续带着 memory_order_consume 的依赖链”。
//
// 可以先把几个概念拆开:
//
// 1. memory_order_consume
//    - 它想表达的是: 只对“依赖于 load 结果的后续读”建立顺序约束。
//    - 比如你先 load 到一个指针 p, 然后访问 p->field, 这个 field 的读取就依赖于 p。
//
// 2. [[carries_dependency]]
//    - 用来告诉编译器: 这个函数参数/返回值会把依赖链继续往外传。
//    - 否则跨函数调用时, 编译器可能看不出来依赖关系想继续保留。
//
// 3. std::kill_dependency(x)
//    - 返回值仍然等于 x
//    - 但语义上告诉编译器: “到这里为止, 不要再把依赖链继续往后传播了”
//
// 重要现实情况:
// - 主流编译器今天通常把 memory_order_consume 当作 memory_order_acquire 处理。
// - 而且像 g++ 这样的编译器还可能直接忽略 [[carries_dependency]] 属性。
// - 所以这个 demo 的运行输出不会因为 kill_dependency 而变不同。
// - 这个例子是在解释“内存模型语义”, 不是在演示肉眼可见的功能差异。

using namespace std::chrono_literals;

struct Published {
	// slot_ptr 的地址来自 consume load 到的对象,
	// 所以随后对 *slot_ptr 的读取可以形成依赖链。
	int *slot_ptr;
	int payload;
};

std::atomic<Published *> g_head{ nullptr };
int g_slots[4] = { 1, 3, 2, 0 };
int g_values[5] = { 10, 20, 30, 40, 50 };
Published g_node;

void print_title(const char *title)
{
	std::cout << "\n=== " << title << " ===\n";
}

void publisher()
{
	// 模拟“先准备数据, 再发布指针”
	g_node.slot_ptr = &g_slots[1]; // 指向值 3
	g_node.payload = 1234;

	std::cout << "publisher: g_node.slot_ptr 指向 g_slots[1], *slot_ptr = "
		  << *g_node.slot_ptr << '\n';
	std::cout << "publisher: 用 release store 发布 g_node 地址\n";

	g_head.store(&g_node, std::memory_order_release);
}

// consume load 从原子变量里读出一个指针。
// 这个指针值本身就是依赖链的起点。
[[carries_dependency]] Published *load_head_consume()
{
	return g_head.load(std::memory_order_consume);
}

// 这里保留依赖链:
// consume 读到的 p -> p->slot_ptr -> *p->slot_ptr -> 返回的 slot
// 调用者如果再拿 slot 去做数组索引, 依赖链会继续传下去。
[[carries_dependency]] int read_slot_keep_dependency(
	Published *p [[carries_dependency]])
{
	return *p->slot_ptr;
}

// 这里杀死依赖链:
// 返回的整数值和上面完全一样, 还是 *p->slot_ptr。
// 但 std::kill_dependency 告诉编译器:
// “这个整数从这里开始, 别再把它当成 consume 依赖链的一部分了。”
int read_slot_kill_dependency(Published *p [[carries_dependency]])
{
	return std::kill_dependency(*p->slot_ptr);
}

// 如果 slot 仍然 carries_dependency, 那么这里用 slot 做数组索引,
// 语义上仍然是在延续那条 consume 依赖链。
int lookup_keep_dependency(int slot [[carries_dependency]])
{
	return g_values[slot];
}

// 如果 slot 来自 read_slot_kill_dependency(), 那么这里虽然查到的值一样,
// 但这一步已经不再被视为 consume 依赖链的一部分。
int lookup_after_kill(int slot)
{
	return g_values[slot];
}

int main()
{
	print_title("1. 发布数据");
	std::thread t(publisher);
	t.join();

	print_title("2. consume load 读取已发布指针");
	Published *p = nullptr;
	while ((p = load_head_consume()) == nullptr)
		std::this_thread::sleep_for(1ms);

	std::cout << "consumer: 拿到 Published* p = " << p << '\n';
	std::cout << "consumer: p->payload = " << p->payload << '\n';
	std::cout << "consumer: p->slot_ptr = " << p->slot_ptr << '\n';
	std::cout << "consumer: *p->slot_ptr = " << *p->slot_ptr << '\n';

	print_title("3. 保留依赖链");
	int slot_keep = read_slot_keep_dependency(p);
	int value_keep = lookup_keep_dependency(slot_keep);
	std::cout << "slot_keep = " << slot_keep << '\n';
	std::cout << "g_values[slot_keep] = " << value_keep << '\n';
	std::cout << "解释: slot_keep 仍然被当作 consume 依赖链的一部分\n";

	print_title("4. 杀死依赖链");
	int slot_kill = read_slot_kill_dependency(p);
	int value_kill = lookup_after_kill(slot_kill);
	std::cout << "slot_kill = " << slot_kill << '\n';
	std::cout << "g_values[slot_kill] = " << value_kill << '\n';
	std::cout << "解释: 数值完全一样, 但从 std::kill_dependency 开始,\n"
		     "      后面的 slot_kill 不再继续携带 consume 依赖链语义\n";

	print_title("5. 一句话总结");
	std::cout << "std::kill_dependency(x) 不会改 x 的值,\n"
		     "它改的是“依赖链是否继续传播”这件事。\n";
	std::cout << "所以它更像一个内存模型层面的“语义剪刀”,\n"
		     "而不是普通业务逻辑里的值变换函数。\n";
}
