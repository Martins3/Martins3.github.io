#include "dynamic-order.hpp"

#include <cassert>

// 看输出时区分三件事：
// - ordered：a.cpp 内、b.cpp 内各自的 1 一定先于 2，但两文件之间不保证顺序。
// - partially-ordered：各翻译单元中 inline 定义顺序一致，故本例 1 一定先于 2。
// - unordered：模板实例 1、2 谁先都不保证，不能从某次输出反推语言规则。
// 换链接顺序能否改变输出属于实现行为；标准不要求必须观察到不同结果。
// unordered 在这个单线程例子中也不是“随机数”或“必须并行执行”。
// 规则：https://timsong-cpp.github.io/cppwp/n4861/basic.start.dynamic

int main()
{
	// 先使用两个翻译单元的函数，兼顾实现选择延迟动态初始化的情况。
	const Order va = variables_a();
	const Order vb = variables_b();
	const Order ma = members_a();
	const Order mb = members_b();

	assert(ordered_a_first < ordered_a_second);
	assert(ordered_b_first < ordered_b_second);
	assert(partial_first < partial_second);
	// 两个翻译单元看到的是同一组模板实例，并非每个文件各有一份。
	assert(va.first == vb.second && va.second == vb.first);
	assert(ma.first == mb.second && ma.second == mb.first);
	assert(vb.first > 0 && vb.second > 0 && vb.first != vb.second);
	assert(mb.first > 0 && mb.second > 0 && mb.first != mb.second);

	std::puts("--- initialization sequence numbers ---");
	std::printf("ordered a: first=%d second=%d\n", ordered_a_first, ordered_a_second);
	std::printf("ordered b: first=%d second=%d\n", ordered_b_first, ordered_b_second);
	std::printf("partial: first=%d second=%d\n", partial_first, partial_second);
	std::printf("unordered variable: <1>=%d <2>=%d\n", vb.first, vb.second);
	std::printf("unordered member:   <1>=%d <2>=%d\n", mb.first, mb.second);
	std::puts("All guaranteed-order checks passed.");
}
