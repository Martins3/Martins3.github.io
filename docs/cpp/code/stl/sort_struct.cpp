// # cpp 中如何做结构体排序
// <!-- 92c9bfc0-60be-4e5a-bb38-a285f12e71a2 -->
#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

struct Student {
	std::string name;
	int score;
};

// score 高的排在前面；score 相同时，按 name 升序排列。
bool student_less(const Student &lhs, const Student &rhs)
{
	if (lhs.score != rhs.score)
		return lhs.score > rhs.score;

	return lhs.name < rhs.name;
}

void sort_struct()
{
	std::vector<Student> students = {
		{ "Bob", 85 },
		{ "Alice", 92 },
		{ "David", 85 },
		{ "Carol", 92 },
	};

	std::sort(students.begin(), students.end(), student_less);

	for (const Student &student : students)
		std::cout << student.name << ' ' << student.score << '\n';
}

bool m(const std::pair<int, int> a, const std::pair<int, int> b)
{
	return a.second > b.second;
}

void sort_pair()
{
	std::vector<std::pair<int, int> > a = {
		std::pair(1, 2),
		std::pair(3, 4),
	};
	sort(a.begin(), a.end(), m);
	for (auto m : a) {
		std::cout << m.first << " " << m.second << std::endl;
	}
}

int main()
{
	sort_struct();
	sort_pair();
}
