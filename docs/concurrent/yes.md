## kernel/sched/membarrier.c syscall

- https://man7.org/linux/man-pages/man2/membarrier.2.html
- https://stackoverflow.com/questions/67380457/understand-membarrier-function-in-linux

以前 fast-path 和 slow-path 都是需要添加 barrier 的。
叫fast-path， 我们不需要添加任何内存barrier指令，slow-path 中调用系统调用


## 这些东西写成文档和自带测试吧

我想这些东西都是知道在搞什么的

- [ ] atomic 指令是自动携带 memory barrier 的吗？
- [ ] x86 为什么又存在 lock prefix，又存在 cas 指令
- [ ] atomic 和 cas 似乎是两个东西啊
- [ ] memory barrier 总是配对的吧

## TSO 中的 store/load 乱序具体是指?
<!-- 762ac400-6a80-43b6-b7f4-346fff031cc1 -->

- store 和 load 的组合有 4 种。分别是 store-store，store-load，load-load 和 load-store。
- TSO 模型中，只存在 store-load 存在乱序，另外 3 种内存操作不存在乱序。

只有是这样模型，是因为
存在 write buffer ，但是没有 read buffer ?

结合 再谈C++原子操作与内存屏障 - icysky的文章 - 知乎
https://zhuanlan.zhihu.com/p/27554789673 其中提到了 invalid queue ，这个是需要理解一下为什么 x86 这么设计

## 问题
如果已经存在了 smp_mb() 还需要使用 barrier() 吗?

不需要，这个是内核的中的实现的，通过 "memory" ，告诉编译器不要乱来。
```c
#define dmb(opt)	asm volatile("dmb " #opt : : : "memory")
```

```c
#define __smp_mb()	asm volatile("lock addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")
```

## llist_for_each 和 llist_for_each_safe 的区别是什么?
<!-- d1b4eab9-e9a6-432b-9090-0a3439bc4691 -->

和锁没有任何的关系，只是是否可以在 list 中删除而已

```c
/**
 * llist_for_each - iterate over some deleted entries of a lock-less list
 * @pos:	the &struct llist_node to use as a loop cursor
 * @node:	the first entry of deleted list entries
 *
 * In general, some entries of the lock-less list can be traversed
 * safely only after being deleted from list, so start with an entry
 * instead of list head.
 *
 * If being used on entries deleted from lock-less list directly, the
 * traverse order is from the newest to the oldest added entry.  If
 * you want to traverse from the oldest to the newest, you must
 * reverse the order by yourself before traversing.
 */
#define llist_for_each(pos, node)			\
	for ((pos) = (node); pos; (pos) = (pos)->next)

/**
 * llist_for_each_safe - iterate over some deleted entries of a lock-less list
 *			 safe against removal of list entry
 * @pos:	the &struct llist_node to use as a loop cursor
 * @n:		another &struct llist_node to use as temporary storage
 * @node:	the first entry of deleted list entries
 *
 * In general, some entries of the lock-less list can be traversed
 * safely only after being deleted from list, so start with an entry
 * instead of list head.
 *
 * If being used on entries deleted from lock-less list directly, the
 * traverse order is from the newest to the oldest added entry.  If
 * you want to traverse from the oldest to the newest, you must
 * reverse the order by yourself before traversing.
 */
#define llist_for_each_safe(pos, n, node)			\
	for ((pos) = (node); (pos) && ((n) = (pos)->next, true); (pos) = (n))
```

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
