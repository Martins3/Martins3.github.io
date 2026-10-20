#include <stdio.h>

// 故意使用 C 也认识的语法和头文件，用同一份代码比较全局初始化限制。
// 在本目录执行 make run-scanf 手动输入；make run 会自动输入 7。
// g++ -std=c++20 -Wall -Wextra dynamic-scanf.cpp -o /tmp/dynamic-scanf.out
// printf '7\n' | /tmp/dynamic-scanf.out
// gcc -std=c17 -pedantic-errors -x c -fsyntax-only dynamic-scanf.cpp
// 最后一个命令应报错：全局 runtime_value 的初始化器不是常量表达式。
// 若把 int runtime_value = read_config(); 移入 main，普通局部变量在 C 中也合法。

int no_initializer;     // 零初始化；Linux/GCC 通常放 .bss
int explicit_zero = 0; // 常量初始化；即使写了初始化器，通常仍放 .bss
int constant_value = 42; // 常量初始化；通常放 .data

int read_config(void)
{
	printf("read_config: enter an integer: ");
	fflush(stdout); // 即使输出重定向，也先显示提示再等待输入。
	int value = 0;
	if (scanf("%d", &value) != 1) {
		puts("input failed; using 0");
		return 0;
	}
	printf("read_config: got %d\n", value);
	return value;
}

// 先在静态阶段零初始化，再调用 read_config() 完成动态初始化。
// 它即使有初始化器，Linux/GCC 也通常为其在 .bss 中安排存储。
int runtime_value = read_config();

int main(void)
{
	printf("main: no_initializer=%d explicit_zero=%d constant_value=%d runtime_value=%d\n",
	       no_initializer, explicit_zero, constant_value, runtime_value);
	return 0;
}
