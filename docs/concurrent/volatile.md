
## 具体问题

2.

为什么 inode->i_link 的访问需要 READ_ONCE

```txt
 History:        #0
 Commit:         4c4f7c19b3c721aed418bc97907b411608c5c6a0
 Author:         Eric Biggers <ebiggers@google.com>
 Committer:      Theodore Ts'o <tytso@mit.edu>
 Author Date:    Thu 11 Apr 2019 04:21:14 AM CST
 Committer Date: Thu 18 Apr 2019 12:43:14 AM CST

 vfs: use READ_ONCE() to access ->i_link

 Use 'READ_ONCE(inode->i_link)' to explicitly support filesystems caching
 the symlink target in ->i_link later if it was unavailable at iget()
 time, or wasn't easily available.  I'll be doing this in fscrypt, to
 improve the performance of encrypted symlinks on ext4, f2fs, and ubifs.

 ->i_link will start NULL and may later be set to a non-NULL value by a
 smp_store_release() or cmpxchg_release().  READ_ONCE() is needed on the
 read side.  smp_load_acquire() is unnecessary because only a data
 dependency barrier is required.  (Thanks to Al for pointing this out.)

 Acked-by: Al Viro <viro@zeniv.linux.org.uk>
 Signed-off-by: Eric Biggers <ebiggers@google.com>
 Signed-off-by: Theodore Ts'o <tytso@mit.edu>
```

测试代码 : ~/data/vn/code/src/concurrent/access-once/

1. 为什么 arch_test_and_set_bit 的参数中需要携带 volatile

```c
arch_test_and_set_bit(long nr, volatile unsigned long *addr)
```

## 基本语法问题

1. 一个volatile 变量传递给一个函数，

```txt
volatile unsigned long addr;
void m(unsigned long m);
```

2. volatile 指针的语义是什么?

例如:

```txt
volatile unsigned long * addr;
void m(unsigned long m);

m(addr);
```

## barrier()
<!-- 33679899-6172-4df8-85d0-9c23d73251d3 -->

```c
# define barrier() __asm__ __volatile__("": : :"memory")
```

> [!NOTE]
> 参考 Deepseeek ，有待验证

`__asm__` 这是 GCC/Clang 提供的关键字，用于在 C/C++ 代码中嵌入汇编指令。

`__volatile__` 这是至关重要的一部分。它告诉编译器**“不要动我”**。
1. 禁止优化删除：编译器不能因为觉得这行代码“没用”（因为它不产生任何输出）就把它优化掉。
2. 禁止重排：编译器不能将这条汇编指令与其他代码进行重排。它必须精确地停留在你放置它的位置。

"" (空的汇编模板) : 这部分是你要插入的汇编代码。在这里，它是空的。这意味着我们不想生成任何实际的 CPU 汇编指令。这很关键，因为它表明这个操作的目标不是 CPU，而是编译器本身。

: (空的输出和输入操作数) :
1. 第一个冒号后面是输出操作数列表，为空，因为我们不从汇编代码中向任何 C 变量写入结果。
2. 第二个冒号后面是输入操作数列表，也为空，因为我们不从任何 C 变量向汇编代码传递值。

"memory" (Clobber 列表) : 这是整个构造的“灵魂”。Clobber 列表用来告知编译器，这段内联汇编可能会修改（clobber）某些寄存器或内存。

当 "memory" 出现在这里时，它向编译器发出了一个非常强烈的信号：“这段代码之后，任何缓存在 CPU 寄存器中的内存值都可能是过时的，不可信任了。”

它的实际作用是什么？
"memory" clobber 会强制编译器做两件核心的事情，从而形成一个“屏障”：
- 刷新寄存器到内存：在执行这条指令之前，如果编译器有一些对内存的修改（比如 x = 1;）还暂存在寄存器中没有写回，它必须先将这些值写回主内存。
- 废弃已缓存的内存值：在执行这条指令之后，如果编译器需要再次访问某个内存地址（比如读取 y 的值），它不能想当然地使用可能已经缓存在寄存器里的旧值，而必须重新从主内存中加载。

最终效果：编译器被禁止将任何内存访问操作（读或写）从屏障的一侧移动到另一侧。

(这个回答非常有道理，关于 gcc inline assembled 的解释，
但是我感觉到奇怪的地方在于，如果想要让代码不要出现编译器乱序，为什么不直接使用 volatile
来精确的控制?

真的需要有使用 barrier() 的位置吗?
)

继续和 __sync_synchronize 做一个对比
https://stackoverflow.com/questions/982129/what-does-sync-synchronize-do

barrier() 和 __sync_synchronize 是一个东西吗?

内核中的 tools/include/io_uring/mini_liburing.h 如何理解?
```txt
#if defined(__x86_64) || defined(__i386__)
#define read_barrier()	__asm__ __volatile__("":::"memory")
#define write_barrier()	__asm__ __volatile__("":::"memory")
#else
#define read_barrier()	__sync_synchronize()
#define write_barrier()	__sync_synchronize()
#endif
```

## 具体问题

1. preempt_disable() 中为什么需要 barrier

```c
#define preempt_disable() \
do { \
	preempt_count_inc(); \
	barrier(); \
} while (0)
```
首先，显然不能让编译器出现指令重排，其次，
这里为什么这里仅仅使用 barrier 就可以了?
因为这个同步不涉及其他的，CPU 的实现可以保证，

这真是一个经典的例子啊

2. 为什么 __rcu_read_lock 使用的中是 barrier() ?

```c
/*
 * Preemptible RCU implementation for rcu_read_lock().
 * Just increment ->rcu_read_lock_nesting, shared state will be updated
 * if we block.
 */
void __rcu_read_lock(void)
{
	rcu_preempt_read_enter();
	if (IS_ENABLED(CONFIG_PROVE_LOCKING))
		WARN_ON_ONCE(rcu_preempt_depth() > RCU_NEST_PMAX);
	if (IS_ENABLED(CONFIG_RCU_STRICT_GRACE_PERIOD) && rcu_state.gp_kthread)
		WRITE_ONCE(current->rcu_read_unlock_special.b.need_qs, true);
	barrier();  /* critical section after entry code. */
}
```
因为 rcu 的本质是，防止在 rcu_read_lock 之后，在上下文切换的时候，没有知道
rcu_read_lock_nesting 不为 0 ，从而放弃掉切换为其他的程序。所以，
他不需要其他的同步。只要让 barrier 后面的代码不要跑到前面去了就可以了。

## 为什么 jiffies 必须标记为 volatile ?
<!-- 05a48690-dbec-4bea-8100-5a44666a6c0d -->

这是一个经典例子。

回忆一下 code/src/m/concurrent/access_once.c 中的例子，
那就是，如果 jiffies 不是 volatile ，那么多次访问 jiffies 是会被合并到一起。

类似的，如果 stop 不去配置为 volatile ，那么这个 access_once
```c
static int stop = 1;
void test5(void)
{
	for (int i = 1; ; i++) {
		if(stop)
			break;
  }
}

// 在另外一个 thread 中设置为 stop ，而这个 stop 很有可能被优化掉。
```

## context_switch 中为什么需要有一个 barrier

```c
	/* Here we just switch the register state and the stack. */
	switch_to(prev, next, prev);
	barrier();

  return finish_task_switch(prev);
```


## `READ_ONCE` and `WRITE_ONCE`
  - https://github.com/google/kernel-sanitizers/blob/master/other/READ_WRITE_ONCE.md

- [WRITE_ONCE and READ_ONCE in linux kernel](https://stackoverflow.com/questions/50589499/write-once-and-read-once-in-linux-kernel)
  - 这么经典的问题居然没有人回答

- [Why the "volatile" type class should not be used](https://github.com/torvalds/linux/blob/master/Documentation/process/volatile-considered-harmful.rst)
  - 文档中说，不应该使用 volatile，但是内核中还是存在这么多的使用的地方

```txt
🧀  ag "\tvolatile" | wc -l
1962
```

## notes
https://lwn.net/Articles/508991/

## [ ] 具体案例
c6ed4d84a2c49de7d6f490144cca7b4a4831fb6e

看不懂
```diff
commit d57f727264f1425a94689bafc7e99e502cb135b5
Author: Vineet Gupta <vgupta@synopsys.com>
Date:   Thu Nov 13 15:54:01 2014 +0530

    ARC: add compiler barrier to LLSC based cmpxchg

    When auditing cmpxchg call sites, Chuck noted that gcc was optimizing
    away some of the desired LDs.

    |       do {
    |               new = old = *ipi_data_ptr;
    |               new |= 1U << msg;
    |       } while (cmpxchg(ipi_data_ptr, old, new) != old);

    was generating to below

    | 8015cef8:     ld         r2,[r4,0]  <-- First LD
    | 8015cefc:     bset       r1,r2,r1
    |
    | 8015cf00:     llock      r3,[r4]  <-- atomic op
    | 8015cf04:     brne       r3,r2,8015cf10
    | 8015cf08:     scond      r1,[r4]
    | 8015cf0c:     bnz        8015cf00
    |
    | 8015cf10:     brne       r3,r2,8015cf00  <-- Branch doesn't go to orig LD

    Although this was fixed by adding a ACCESS_ONCE in this call site, it
    seems safer (for now at least) to add compiler barrier to LLSC based
    cmpxchg

    Reported-by: Chuck Jordan <cjordan@synopsys,com>
    Cc: <stable@vger.kernel.org>
    Acked-by: Peter Zijlstra (Intel) <peterz@infradead.org>
    Signed-off-by: Vineet Gupta <vgupta@synopsys.com>

diff --git a/arch/arc/include/asm/cmpxchg.h b/arch/arc/include/asm/cmpxchg.h
index 03cd6894855d..90de5c528da2 100644
--- a/arch/arc/include/asm/cmpxchg.h
+++ b/arch/arc/include/asm/cmpxchg.h
@@ -25,10 +25,11 @@ __cmpxchg(volatile void *ptr, unsigned long expected, unsigned long new)
 	"	scond   %3, [%1]	\n"
 	"	bnz     1b		\n"
 	"2:				\n"
-	: "=&r"(prev)
-	: "r"(ptr), "ir"(expected),
-	  "r"(new) /* can't be "ir". scond can't take limm for "b" */
-	: "cc");
+	: "=&r"(prev)	/* Early clobber, to prevent reg reuse */
+	: "r"(ptr),	/* Not "m": llock only supports reg direct addr mode */
+	  "ir"(expected),
+	  "r"(new)	/* can't be "ir". scond can't take LIMM for "b" */
+	: "cc", "memory"); /* so that gcc knows memory is being written here */

 	return prev;
 }
```

```diff
commit c6ed4d84a2c49de7d6f490144cca7b4a4831fb6e
Author: Bang Li <libang.linuxer@gmail.com>
Date:   Sat Mar 19 10:03:16 2022 +0800

    ARC: remove redundant READ_ONCE() in cmpxchg loop

    This patch reverts commit 7082a29c22ac ("ARC: use ACCESS_ONCE in cmpxchg
    loop").

    It is not necessary to use READ_ONCE() because cmpxchg contains barrier. We
    can get it from commit d57f727264f1 ("ARC: add compiler barrier to LLSC
    based cmpxchg").

    Signed-off-by: Bang Li <libang.linuxer@gmail.com>
    Signed-off-by: Vineet Gupta <vgupta@kernel.org>

diff --git a/arch/arc/kernel/smp.c b/arch/arc/kernel/smp.c
index 383fefee2ae5..d947473f1e6d 100644
--- a/arch/arc/kernel/smp.c
+++ b/arch/arc/kernel/smp.c
@@ -274,7 +274,7 @@ static void ipi_send_msg_one(int cpu, enum ipi_msg_type msg)
 	 * and read back old value
 	 */
 	do {
-		new = old = READ_ONCE(*ipi_data_ptr);
+		new = old = *ipi_data_ptr;
 		new |= 1U << msg;
 	} while (cmpxchg(ipi_data_ptr, old, new) != old);
```

## 为什么将 ACCESS_ONCE 转换为 READ_ONCE
tools/perf: Convert ACCESS_ONCE() to READ_ONCE()


## 官方的参考
https://lwn.net/Articles/799218/#Access-Marking%20Policies
仔细看了下，意思很简单，就是没有 READ_ONCE ，编译器就会优化，导致 compiler 会将 kernel 中的代码


# volatile

## [Nine ways to break your systems code using volatile](https://blog.regehr.org/archives/28) 读后感

简单来说，如果一个变量被 volatile 修饰，那么编译器不能优化对于其的读写操作，必须生成对应的指令。
- volatile 只能保证有指令生成，但是编译器可以调度这些指令
  - > Accesses to non-volatile objects are not ordered with respect to volatile accesses. [^1]
  - 但是 volatiles accesses 不会被 reordered [^2]
- 但是 CPU 未必真的会从内存中访问
- CPU 的执行对于指令的执行顺序也可能是乱序的

时钟计数器是一个经典的 const volatile 变量[^3]
```c
extern const volatile int real_time_clock;
```

勘误，下面的这个说法应该是错误了，从 stackoverflow 的这个回答[^4] 和 https://godbolt.org/ 的测试显示，asm volatile ("" : : : "memory") 只是阻止了指令调度，而没有进行将寄存器写回内存的操作。
> The effect is that the compiler dumps all registers to RAM before the barrier and reloads them afterwards.  Moreover, code motion is not permitted around the barrier in either direction.

## asm volatile

https://stackoverflow.com/questions/14950614/working-of-asm-volatile-memory

- [ ] 好吧，我不是非常理解，为什么 x86 会存在 mfence 和 lfence 的啊?
  - [ ] 不是说好的 x86 是 strong model, 所以 asm volatile("":::memory) 不用生成什么，那么为什么还存在 mfence
  - https://stackoverflow.com/questions/12183311/difference-in-mfence-and-asm-volatile-memory
  - https://stackoverflow.com/questions/27595595/when-are-x86-lfence-sfence-and-mfence-instructions-required

- 反正 CPU 都可以进行调度，那么 asm volatile 在 x86 中什么都不生成，那么有个什么作用啊

## 其他的小问题
[stackoverflow : What does—or did—"volatile void function( ... )" do?](https://stackoverflow.com/questions/14288603/what-does-or-did-volatile-void-function-do)

> volatile void as a function return value in C (but not in C++) is equivalent to __attribute__((noreturn)) on the function and tells the compiler that the function never returns.

[可以使用 volatile 或者 const 修饰 static 函数吗 ?](https://stackoverflow.com/questions/3078237/defining-volatile-class-object)

```c
bool static push(struct Data element) volatile;  // 这种形式，不可以
bool static volatile push(struct Data element); // 这种形式，可以
```

## 一个例子说明白，这个就是给 compiler 用的
https://github.com/google/kernel-sanitizers/blob/master/kcsan/LPC2020-KCSAN.pdf

[^1]: https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html
[^2]: https://stackoverflow.com/questions/14785639/may-accesses-to-volatiles-be-reordered
[^3]: https://stackoverflow.com/questions/4592762/difference-between-const-const-volatile

## 仔细理解这个
- https://lwn.net/Articles/799218/#Access-Marking%20Policies

其实，就是，如果是共享变量，那么就必须在

## 有待确认
- 没有必要在锁中间使用 WRITE_ONCE / READ_ONCE 访问普通内存
  - 因为上锁了之后，相当于单线程了，WRITE_ONCE / READ_ONCE 访问内存的话，就是多线程用的

## 有趣的例子

非常合理
```c
static void
qemu_cfg_dma_transfer(void *address, u32 length, u32 control)
{
    QemuCfgDmaAccess access;

    access.address = cpu_to_be64((u64)(u32)address);
    access.length = cpu_to_be32(length);
    access.control = cpu_to_be32(control);

    barrier();

    outl(cpu_to_be32((u32)&access), PORT_QEMU_CFG_DMA_ADDR_LOW);

    while(be32_to_cpu(access.control) & ~QEMU_CFG_DMA_CTL_ERROR) {
        yield();
    }
}
```


## READ_ONCE 似乎加的很随意，这个是靠 KCSAN 找到的吗?

这里阅读 slab->slabs 为什么上面那个不需要 READ_ONCE，下面那个需要

```c
static ssize_t slabs_cpu_partial_show(struct kmem_cache *s, char *buf)
{
	int objects = 0;
	int slabs = 0;
	int cpu __maybe_unused;
	int len = 0;

#ifdef CONFIG_SLUB_CPU_PARTIAL
	for_each_online_cpu(cpu) {
		struct slab *slab;

		slab = slub_percpu_partial(per_cpu_ptr(s->cpu_slab, cpu));

		if (slab)
			slabs += slab->slabs;
	}
#endif

	/* Approximate half-full slabs, see slub_set_cpu_partial() */
	objects = (slabs * oo_objects(s->oo)) / 2;
	len += sysfs_emit_at(buf, len, "%d(%d)", objects, slabs);

#ifdef CONFIG_SLUB_CPU_PARTIAL
	for_each_online_cpu(cpu) {
		struct slab *slab;

		slab = slub_percpu_partial(per_cpu_ptr(s->cpu_slab, cpu));
		if (slab) {
			slabs = READ_ONCE(slab->slabs);
			objects = (slabs * oo_objects(s->oo)) / 2;
			len += sysfs_emit_at(buf, len, " C%d=%d(%d)",
					     cpu, objects, slabs);
		}
	}
#endif
	len += sysfs_emit_at(buf, len, "\n");

	return len;
}
SLAB_ATTR_RO(slabs_cpu_partial);
```
可以看下和最近代码的变化，发现 linke li 搞了几个相关的修改，
把这里的 READ_ONCE 又修改为
data_race ，这里真的让人很懵逼了。

执行命令 check linke li 可以看看这个哥们
为什么在检查了那么多的 date_race



## 首先这个定义看懂了没有
```c
/*
 * Use __READ_ONCE() instead of READ_ONCE() if you do not require any
 * atomicity. Note that this may result in tears!
 */
#ifndef __READ_ONCE
#define __READ_ONCE(x)	(*(const volatile __unqual_scalar_typeof(x) *)&(x))
#endif

#define READ_ONCE(x)							\
({									\
	compiletime_assert_rwonce_type(x);				\
	__READ_ONCE(x);							\
})

#define __WRITE_ONCE(x, val)						\
do {									\
	*(volatile typeof(x) *)&(x) = (val);				\
} while (0)

#define WRITE_ONCE(x, val)						\
do {									\
	compiletime_assert_rwonce_type(x);				\
	__WRITE_ONCE(x, val);						\
} while (0)
```
1. 为什么这个定义没有对称
2. 如何理解这个注释?

## 为什么这个还会被 gcc 来警告我?
```txt
diff --git a/src/include/rwonce.h b/src/include/rwonce.h
index f6288715d0ea..6a481b403c38 100644
--- a/src/include/rwonce.h
+++ b/src/include/rwonce.h
@@ -1,7 +1,7 @@
 #ifndef __ASM_GENERIC_RWONCE_H
 #define __ASM_GENERIC_RWONCE_H

-#define READ_ONCE(x) ({ (*(const volatile typeof(x) *)&(x)); })
+#define READ_ONCE(x) (*(const volatile typeof(x) *)&(x))

 #define WRITE_ONCE(x, val)                           \
 	do {                                         \
```

> [!NOTE]
> 参考 Deepseeek ，有待验证

({ ... }) 是 GCC 特有的 语句表达式：它允许在表达式中嵌入复合语句（即花括号内的多条语句），并以最后一条表达式的值作为整个语句表达式的值。
ISO C（即标准 C 语言，如 C99、C11）不支持这种语法，因此当你开启 -Wpedantic 时，编译器会提示

## 为什么 __list_add 中需要添加 WRITE_ONCE
<!-- 3c68f3af-2af3-433e-8989-0987394c18eb -->

```c
static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	if (!__list_add_valid(new, prev, next))
		return;

	next->prev = new;
	new->next = next;
	new->prev = prev;
	WRITE_ONCE(prev->next, new);
}
```

```c
next->prev = new;   // 1. 修复后继的 back 指针
new->next  = next;  // 2. 初始化 new 的 next
new->prev  = prev;  // 3. 初始化 new 的 prev
WRITE_ONCE(prev->next, new); // 4. “发布” new 到链表中
```
所以，我希望这个发布之后，你可以就可以观察到正确的

内核中使用 WRITE_ONCE() 是为了：
- 让调试工具（KCSAN、lockdep）更容易识别竞态
- 明确表达“这里是一个发布点”

###  这个函数会被 rcu 调用吗?


在 RCU 场景中，对应代码通常是：
```c
rcu_assign_pointer(prev->next, new);
```

其内部等价于：
```c
smp_wmb();
WRITE_ONCE(prev->next, new);
```

rcu 有自己的函数:
```c
static inline void __list_add_rcu(struct list_head *new,
		struct list_head *prev, struct list_head *next)
{
	if (!__list_add_valid(new, prev, next))
		return;

	new->next = next;
	new->prev = prev;
	rcu_assign_pointer(list_next_rcu(prev), new);
	next->prev = new;
}
```
这里我没有想到为什么这里，

### 其他 CPU 上的遍历代码通常，也就是说，遍历需要 READ_ONCE ?

```c
p = READ_ONCE(prev->next);
```

include/linux/list.h 中使用的 READ_ONCE 的地方继续看看吧，
想了想，似乎 READ_ONCE 和 WRITE_ONCE 也是需要成对使用的



## 真的吗?
### 编译器拆分指针写入（32 位系统尤其危险）

在 32bit 架构或特殊 ABI 下，一个指针写：

```c
prev->next = new;
```

**可能被拆成两个 16 位写操作**。
如果此时并发读取：

```c
tmp = prev->next;
```

可能读到：

* 低 16 位是新值
* 高 16 位是旧值

得到一个 **完全非法地址**

`WRITE_ONCE` 明确禁止这种拆分。

## READ_ONCE(), WRITE_ONCE(), but not for Rust
<!-- eccc5bff-228b-4be5-a882-f294223fbff6 -->

的确经典:

原文: [READ_ONCE(), WRITE_ONCE(), but not for Rust](https://lwn.net/Articles/1053142/)
两个翻译:
- https://mp.weixin.qq.com/s/Mu0UticJbkOOouRgG5YYGg
- https://mp.weixin.qq.com/s/7HcXca9Agmoq5VkVddboIQ

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
