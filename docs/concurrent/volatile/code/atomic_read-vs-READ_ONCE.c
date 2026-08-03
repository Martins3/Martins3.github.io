#include <stdlib.h>
/*
 * # atomic_read 和 access_once 的区别是什么?
 * <!-- b8634be0-781c-4f0b-8207-a7899bc028b2 -->
 *
 * 2026-05-03 简单来说，如果对于一个变量只有 atomic read / atomic write ，而且
 * access 的 memory model 是 relaxed 的，那么是没什么区别的。
 *
 * 但是，atomic 还是可以有其他的作用
 *
 * docs/qemu/thread/atomic.md 中我们存在一个疑惑，
 * 那就是 atomic_read 和 volatile 到底有什么区别?
 *
 * 理解下来，atomic_read 和 volatile 的区别就是，atomic_read 会考虑
 *  machine-word sized and properly aligned 下依旧是 atomic 的
 *
 * 所以可以测试一下不对齐的 atomic_read 访问的效果?
 *
 * 也就是说，其实就我们考虑的场景，64bit 内核，正常的程序下，atomic_read 和 READ_ONCE 就是等价的。
 */

int main(int argc, char *argv[])
{
	return EXIT_SUCCESS;
}
