#include <bits/stdc++.h>
#include <cstdlib>
#include <vector>

int parttion(std::vector<int> &nums, auto begin, auto end)
{
	int m = nums[end];
	int i = begin;
	int j = end - 1;
	while (true) {
		// 需要防止出现越界，所以需要 i <= j
		// TODO 这里必须是 i <= j 而不是 i < j
		while (i <= j && nums[i] <= m)
			i++;
		while (i <= j && nums[j] > m)
			j--;
		// 已经出现交错了
		// i > j 的情况
		// 3 8 [5]
		//
		// 3 3 8 [5]
		if (i >= j)
			break;

		std::swap(nums[i], nums[j]);
	}
	// 出现了交错，这里是不会有问题的
	// i 和 j 最多交错一位，这个时候 i 上
	// TODO 这里为什么不可以返回 j ?
	std::swap(nums[i], nums[end]);
	return i;
}

void qsort(std::vector<int> &nums, auto begin, auto end)
{
	if (begin >= end)
		return;
	auto p = parttion(nums, begin, end);
	qsort(nums, begin, p - 1);
	qsort(nums, p + 1, end);
}

void qsort(std::vector<int> &nums)
{
	if (nums.empty())
		return;
	qsort(nums, 0, nums.size() - 1);
}

// Hoare 划分：处理闭区间 [begin, end]，调用时要求 begin < end。
int hoare_partition(std::vector<int> &nums, int begin, int end)
{
	const int pivot = nums[begin + (end - begin) / 2];
	int i = begin - 1;
	int j = end + 1;
	while (true) {
		// TODO 什么意思?
		// 相等时也停下；每轮先移动，避免重复元素导致死循环。
		do {
			++i;
		} while (nums[i] < pivot);
		do {
			--j;
		} while (nums[j] > pivot);
		if (i >= j){
			// TODO 为什么这里是 return j ?
			return j;
		}
		std::swap(nums[i], nums[j]);
	}
}

void qsort_hoare(std::vector<int> &nums, int begin, int end)
{
	if (begin >= end)
		return;
	const int p = hoare_partition(nums, begin, end);
	// p 是分界点，不是 pivot 的最终位置，左区间必须包含 p。
	// 不在纠结于划分为三个范围了
	qsort_hoare(nums, begin, p);
	qsort_hoare(nums, p + 1, end);
}

void qsort_hoare(std::vector<int> &nums)
{
	if (nums.empty())
		return;
	qsort_hoare(nums, 0, static_cast<int>(nums.size()) - 1);
}

void check_result(const std::string &name, const std::vector<int> &input,
		  const std::vector<int> &actual,
		  const std::vector<int> &expected)
{
	if (actual == expected)
		return;

	std::cerr << "FAIL: " << name << '\n';
	auto print = [](const char *label, const std::vector<int> &nums) {
		std::cerr << label;
		for (int n : nums)
			std::cerr << ' ' << n;
		std::cerr << '\n';
	};
	print("input:   ", input);
	print("expected:", expected);
	print("actual:  ", actual);
	std::exit(EXIT_FAILURE);
}

void test_sort(const std::string &name, const std::vector<int> &input)
{
	auto actual = input;
	auto expected = input;
	std::sort(expected.begin(), expected.end());
	qsort(actual);
	check_result(name, input, actual, expected);
	actual = input;
	qsort_hoare(actual);
	check_result("Hoare: " + name, input, actual, expected);
}

void test_subrange(const std::string &name, const std::vector<int> &input,
		   int begin, int end)
{
	auto actual = input;
	auto expected = input;
	if (begin < end)
		std::sort(expected.begin() + begin, expected.begin() + end + 1);
	qsort(actual, begin, end);
	// Compare the entire vector to also check that the outside stays unchanged.
	check_result(name, input, actual, expected);
	actual = input;
	qsort_hoare(actual, begin, end);
	check_result("Hoare: " + name, input, actual, expected);
}

int main()
{
	std::size_t count = 0;
	auto run = [&count](const std::string &name,
			    const std::vector<int> &input) {
		test_sort(name, input);
		++count;
	};
	const int low = std::numeric_limits<int>::min();
	const int high = std::numeric_limits<int>::max();
	run("empty", {});
	run("singleton", { 42 });
	run("singleton minimum", { low });
	run("singleton maximum", { high });
	run("two ascending", { 1, 2 });
	run("two descending", { 2, 1 });
	run("two equal", { 1, 1 });
	run("negative values", { -3, -1, -5, -2, -4 });
	run("mixed signs", { 0, -1, 1, -2, 2, 0 });
	run("integer extremes", { high, low, 0, high, -1, low, 1 });
	run("pivot minimum", { 3, 2, 4, 1 });
	run("pivot maximum", { 3, 1, 2, 4 });
	run("duplicate pivot", { 2, 3, 2, 1, 2, 3, 2 });
	run("alternating", { 1, 0, 1, 0, 1, 0, 1, 0 });

	std::vector<int> ordered(2048);
	std::iota(ordered.begin(), ordered.end(), -1024);
	run("ascending", ordered);
	std::reverse(ordered.begin(), ordered.end());
	run("descending", ordered);
	std::swap(ordered.front(), ordered.back());
	run("nearly descending", ordered);
	run("all equal", std::vector<int>(2048, 7));

	const std::vector<int> range_input = { high, 3, -1, 3, 0, low };
	auto run_range = [&](const std::string &name, int begin, int end) {
		test_subrange(name, range_input, begin, end);
		++count;
	};
	run_range("middle subrange", 1, 4);
	run_range("prefix subrange", 0, 4);
	run_range("suffix subrange", 1, 5);
	run_range("singleton subrange", 2, 2);
	run_range("empty subrange", 3, 2);
	run_range("empty prefix", 0, -1);
	run_range("empty suffix", 6, 5);

	// Exhaust every array of length 0..8 over {-1, 0, 1}.
	int combinations = 1;
	for (int length = 0; length <= 8; ++length) {
		for (int code = 0; code < combinations; ++code) {
			std::vector<int> input(length);
			int digits = code;
			for (int &n : input) {
				n = digits % 3 - 1;
				digits /= 3;
			}
			run("exhaustive length=" + std::to_string(length) +
				    " code=" + std::to_string(code),
			    input);
		}
		combinations *= 3;
	}

	// A fixed seed makes failures reproducible within the same toolchain.
	std::mt19937 rng(0x51450);
	std::uniform_int_distribution<int> lengths(0, 512);
	std::uniform_int_distribution<int> values(low, high);
	std::uniform_int_distribution<int> duplicates(-3, 3);
	for (int iteration = 0; iteration < 2000; ++iteration) {
		std::vector<int> input(lengths(rng));
		for (int &n : input)
			n = iteration % 2 == 0 ? values(rng) : duplicates(rng);
		run("random iteration=" + std::to_string(iteration), input);
	}

	std::cout << "PASS: " << count << " sorting cases for each of 2 algorithms\n";
	return 0;
}
