// https://stackoverflow.com/questions/54585/when-should-you-use-a-class-vs-a-struct-in-c
// struct vs class
#include <iostream>
typedef struct Student {
	int age;
} S;

class BigStudent {
	int age;
} S2;

void Student()
{
} // 正确，定义后 "Student" 只代表此函数

// void S() {} // 错误，符号 "S" 已经被定义为一个 "struct Student" 的别名

int main()
{
	Student();
	struct Student me; // 或者 "S me";
	std::cout << me.age << std::endl;
	return 0;
}
