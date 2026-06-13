# perfbook 阅读思考

## 并发数据结构

stack 有什么并发可言吗？

queue : 如果是 single consumer / single producer 有 lockess 吗?

如果是多个 consumer 和 producer 呢

maple tree 的并发是什么?

rb tree 的并发是什么?
	- 本质还是链表的这个并发类似的

可以证明链表多个 write 一定是不行的
	- 这个容易


### 容易的三个问题

1. seqlock 的实现
3. 什么时候应该使用 seqlock ，他的好处和限制是什么?
	4.  __d_lookup 细节继续分析理解
5. rcu linked list / hash table 和普通的 linked list 的区别是什么?

7. chapters/14 中的高级同步技术的确是非常好用的，lock free 的几个基本概念
8. read once 和 write once 到底有什么奇怪的，彻底整理一下吧，用 lwn 的东西
10. 找到那个 cst 的例子
11. 为什么内核需要定义 LKMM ，LKMM 的具体定义是什么?
12. 继续看看 refcont_t 和 atomic_t 的使用说明
13. 把 cpp 的 demo 都一个个看看吧
14. hazard pointer 是做什么的?

### 不容易的问题
- 写一个 GPU 角度的对比
	- GPU 存在 cache 一致性和 memory model 吗?


## chapter 5
5.22 的扩展思考

update_balloon_stats 中调用

```c
	all_vm_events(events);
	si_meminfo(&i);

	available = si_mem_available();
	caches = global_node_page_state(NR_FILE_PAGES);
```

1. 这里的 all_vm_events 只是需要持有 cpu_hotplug_lock

之后的 read 就只是:
```c
		struct vm_event_state *this = &per_cpu(vm_event_states, cpu);
```

这里为什么不需要 READ_ONCE 啊?

2. global_node_page_state 中的 read 为什么又是 atomic 的
```c
unsigned long global_node_page_state_pages(enum node_stat_item item)
{
	long x = atomic_long_read(&vm_node_stat[item]);
#ifdef CONFIG_SMP
	if (x < 0)
		x = 0;
#endif
	return x;
}
```



```c
#define _GNU_SOURCE /* See feature_test_macros(7) */
#include <poll.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  int a = poll(0, 0, 100000000);
  printf("[martins3:%s:%d] %d\n", __FUNCTION__, __LINE__, a);
  return 0;
}
```

# 想到的问题

1. READ_ONCE 只是保证 read 不会消失，还需要保证指令重排吗？

到了 cache 就存在 cache 一致性，
问题是，提交给 cache 的顺序和代码的顺序可能不同。

一旦向 cache 提交，其他的 core 都应该立刻可见才对。

忽然感觉 cache 好慢，为了 cache 一致性。

但是，痛苦的问题是:
- 两个 io 操作，A , B,  不同的 core 看到的修改顺序可能不同，例如 core 1 看到是 A B，但是 core 2 看到的是 B A

## 仔细阅读

> Unfortunately, these intuitions break
down completely in code that instead uses weakly ordered
atomic operations and memory barriers

什么叫作 weakly ordered atomic operations ?

### 先看懂这个回答
[Atomic operations, std::atomic<> and ordering of writes](https://stackoverflow.com/questions/32384901/atomic-operations-stdatomic-and-ordering-of-writes)

> Fun fact: signed atomic types guarantee two's complement wraparound, unlike normal signed types.
C and C++ still cling to the idea of leaving signed integer overflow undefined in other cases.
Some CPUs don't have arithmetic right shift, so leaving right-shift of negative numbers undefined makes some sense,
but otherwise it just feels like a ridiculous hoop to jump through now that all CPUs use 2's complement and 8bit bytes. </rant>

- [ ] 这段没看懂 到时候再看吧

-> ifso 1

## chapter 3
In particular, adding an instruction to a tight loop can sometimes actually cause execution to speed up

You should partition first, batch second, weaken third, and code fourth.

## 6.5 Beyond Partitioning

一时不知道他要说什么? 这个 chapter 先不看了吧

https://emmilco.github.io/path_finder/

https://www.usenix.org/system/files/conference/hotpar12/hotpar12-final18.pdf

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
