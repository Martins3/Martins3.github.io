# 梳理一下多核的基本生存法则
<!-- ffbbce0f-0e1b-4e7b-819c-434a1e2f1f3e -->

## 首先理清楚数据流

## 先从简单的开始

### memory model 的低配版本

在读写这个变量的时候上锁，pthread 库不会有问题的

## 使用库，而非是自己的 hacking 操作
可以用什么库?

## tab 是辅助思考的好工具，无非就是块块移动来，移动去
```txt
main loop thread        uffd thread / swap out thread

                        lock vm_pool
                        find_vm
                        do business with vm (uffd msg or swap out)
                        unlock vm_pool
lock vm_pool
delete from vm_pool
free vm
unlock vm_pool
                        lock vm_pool
                        find_vm
                        find_vm (vm not found)
                        unlock vm_pool
```

## 常用设计模拟
常用设计模式是什么? (生产者，消费者模式如何上锁)

如何防止出现错误?

### 如何正确的释放资源
现在我有两个线程，其工作如下:
1. 第一个 thread (A) 来负责维护一组资源，例如 struct vm [1024] ，包括添加，删除释放
2. 第二个 thread (B) 查询 struct vm [1024] ，然后使用 vm 。

看看内核的经典操作，在访问数据结构的时候上锁，找到之后，增加引用计数，然后释放锁:
```c
static struct sock *unix_find_socket_byinode(struct inode *i)
{
	unsigned int hash = unix_bsd_hash(i);
	struct sock *s;

	spin_lock(&bsd_socket_locks[hash]);
	sk_for_each_bound(s, &bsd_socket_buckets[hash]) {
		struct dentry *dentry = unix_sk(s)->path.dentry;

		if (dentry && d_backing_inode(dentry) == i) {
			sock_hold(s);
			spin_unlock(&bsd_socket_locks[hash]);
			return s;
		}
	}
	spin_unlock(&bsd_socket_locks[hash]);
	return NULL;
}
```
为什么这里不存在的。

### 数组中分配是需要 lock 的

分配:
```txt
for i in arr:
	if(test_and_set(arr[i].allocated))
		获取到了
```

释放:
atomic_set_true(arr[i].allocated)

其实这个问题没有想象的那么复杂，这个数组中就是一个元素，然后为了
让这个分配是自动的，那么会如何操作? 其实就是一个简单的 test_and_set 就可以了。

## perfbook 经典思考
<!-- 1bfcbf87-b000-4142-b88e-6c25d3445c80 -->

Partition first, batch second, weaken third, and code fourth
然后结合这个章节中的内容理解下:
docs/concurrent/perfbook/autoread/chapters/6/analysis.md

countitng 中经典的四个问题:
1. **网络包统计问题**：每秒数百万次更新，但每五秒才读取一次，允许近似值。
2. **近似结构分配限制**：需要快速判断是否超过限制，允许一定误差。
3. **精确结构分配限制**：必须精确知道何时达到零，用于资源释放。
4. **可移除 I/O 设备引用计数**：平时不关心具体数值，只在卸载时需要精确知道是否为零。

近似结构分配限制的实际使用是什么?

可移除 I/O 设备引用计数 和 精确结构分配限制 在设计上有什么区别?


似乎，各种一致性，在分布式中没理解，然后在 memory model 中也总是没理解啊

docs/concurrent/perfbook/autoread/chapters/8/readme.md
中为什么又重新提到了 Partition bacthing 和 weakenging 的
和 data ownership 有什么关系吗?


原来延迟处理有这么多技术啊:
1. **引用计数（Reference Counting）**
2. **Hazard Pointers（危险指针）**
3. **Sequence Locks（顺序锁）**
4. **Read-Copy Update (RCU)**


docs/concurrent/perfbook/autoread/chapters/10/readme.md
这一章，每一次看都是非常懵逼的，全部都是用的一个数据结构
来分析的，而且全部没思考为什么需要如此。
而且在内核中，我发现并没有那么多的数据结构是完全并行的。

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
