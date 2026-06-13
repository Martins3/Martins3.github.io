/* 
 * 本来想要测试一个并发提交相互有依赖的 io 的:
 * io_uring_prep_renameat
 * io_uring_prep_readv2
 */

#include <stdlib.h>
int main(int argc, char *argv[])
{
	return EXIT_SUCCESS;
}
