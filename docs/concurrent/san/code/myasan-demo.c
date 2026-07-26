/*
 * myasan-demo.c —— 被插桩的一方, 扮演 "内核里的驱动代码".
 *
 * 用 -fsanitize=kernel-address 编译: 编译器把下面每一次内存访问都
 * 变成 __asan_load/storeN_noabort() 调用, 检查逻辑在 myasan.c 里.
 * 正常访问安静通过, 三类错误各报告一次 (noabort, 报告完继续跑).
 */
#include <stdio.h>
#include <string.h>

void *myasan_malloc(size_t size);
void myasan_free(void *ptr);

int g_buf[8]; /* 全局 redzone 由 __asan_register_globals() 在 main 前布好 */

/* 故意不用 static 且禁止内联: -rdynamic 下 backtrace 里能看到函数名 */
__attribute__((noinline)) void heap_overflow(void)
{
	int *p = myasan_malloc(8 * sizeof(int));

	for (int i = 0; i <= 8; i++) /* i == 8 踩到后 redzone */
		p[i] = i;
	myasan_free(p);
}

__attribute__((noinline)) void use_after_free(void)
{
	char *p = myasan_malloc(13);

	p[12] = 'a'; /* 合法: 末尾 granule 前 5 字节可访问, 不报 */
	myasan_free(p);
	p[0] = 'b'; /* 整个 chunk 已 poison 成 0xfd, 报 use-after-free */
}

__attribute__((noinline)) void global_overflow(void)
{
	for (int i = 0; i <= 8; i++) /* i == 8 踩 g_buf 的 redzone */
		g_buf[i] = i;
}

int main(int argc, char **argv)
{
	if (argc == 2 && strcmp(argv[1], "heap-overflow") == 0)
		heap_overflow();
	else if (argc == 2 && strcmp(argv[1], "uaf") == 0)
		use_after_free();
	else if (argc == 2 && strcmp(argv[1], "global") == 0)
		global_overflow();
	else {
		heap_overflow();
		use_after_free();
		global_overflow();
	}
	printf("==myasan== done\n");
	return 0;
}
