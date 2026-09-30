#include <stdlib.h>
/*
 *
- 目录的时候，如果这个目录正在被删掉怎么办?
  - 如果两个 process 都是删同一目录，如何办?
4. 一个 process p 在 a/b/c 下 ，那么当 a 被删除了，p 可以继续访问 c 下文件吗?
   下的文件吗，可以继续创建文件吗，当 p 退出之后，
第二个问题: 一个 process P 正好打开了一个文件 f ，f 被删了，P 关闭 f 之后，f 还在吗?

简单考虑一下，rsync 该如何操作?

1. 提交 fsstat 获取文件的时间戳
1. 可以使用 io uring 同时提交文件打开的功能
2. 获取到目录，立刻提交 readdir ，然后继续提交打开
3. 对于打开的文件提交 io ，然后计算 hash

- 文件被删掉之后，继续 io ，这些 io 写入到哪里，会直接被抛弃掉吗?

一个经常观察到的现象，例如在 /sys/module/sfc/parameters 中，如果我把模块 drop 掉，
shell 还是在当前的目录中，也就是 /sys/module/sfc/parameters 还存在，但是如果

*/

int main(int argc, char *argv[])
{
	return EXIT_SUCCESS;
}
