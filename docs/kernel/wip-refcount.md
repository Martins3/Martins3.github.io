## 文件系统
分析的很有趣， https://zhuanlan.zhihu.com/p/93142176

但是有两个问题:
1. inode 中的 i_count 什么时候超过 1
2. bash 正在一个文件夹中，但是文件夹删除了，cd .. 的结果什么?

- [ ] 一个文件打开的时候，其 dentry 和 inode 可能被回收吗?

## 补充一下其他的
```c
static inline struct task_struct *get_task_struct(struct task_struct *t)
{
	refcount_inc(&t->usage);
	return t;
}
```

## unlink
让 inode ref 减去 1 ，当没有进程 open 之后，那么就会自动删除。

## refcount 自己本身就可以作为一个锁

vi_get 和 vi_put

## refcount 在内核中会成为一个通知机制吗?

## cgroup 专门有一个文件来验证 refcount


```txt
  Linux 内核里对 refcount 的设计不是“统一一个计数器 everywhere”，而是分层做
  的：先有通用安全基元 refcount_t，再在它上面封装对象生命周期管理 kref，对超
  高并发场景单独做 percpu_ref，对 dentry 这类“锁和引用计数强相关”的热点对象
  再做 lockref。也就是说，内核把“引用计数”拆成了安全性、生命周期语义、扩展
  性、缓存热点四个维度分别设计。

  1. 基础层：refcount_t 是给“对象生存期”专门做的，不是普通 atomic_t

  refcount_t 本质上还是包了一层 atomic_t，但 API 被刻意收窄，只保留适合引用
  计数的操作，include/linux/refcount_types.h:8 和 Documentation/core-api/
  refcount-vs-atomic.rst:10 说明了这个目标。

  它最关键的设计点有三个：

  - 溢出/下溢不回绕，而是“饱和 + WARN”，宁可泄漏，也不允许因为计数回绕导致
    UAF。这在 include/linux/refcount.h:11 和 lib/refcount.c:13 写得很清楚。
  - 内存序按“引用计数真正需要的最小语义”来定，而不是一律 full barrier。增引
    用通常是 relaxed；减引用是 release；dec_and_test() 在归零成功时再补
    acquire，这在 include/linux/refcount.h:60 和 Documentation/core-api/
    refcount-vs-atomic.rst:113 可以看到。
  - 明确支持“从 0 以外获取引用”的场景，比如 refcount_inc_not_zero() /
    _acquire()，用于 RCU/lockless lookup 后安全地“捞活对象”，见 include/
    linux/refcount.h:320。

  这说明内核把 refcount_t 设计成“对象 lifetime safety primitive”，而不是普通
  整数原子操作。

  2. 对象生命周期层：kref 把“归零时释放对象”固定成模式

  kref 就是一层很薄的包装，内部直接持有 refcount_t，见 include/linux/
  kref.h:19。但它把最常见的对象生命周期模式固定下来了：

  - kref_init() 初始化为 1，include/linux/kref.h:25
  - kref_get() 增引用，include/linux/kref.h:39
  - kref_put() 最后一个引用归零时调用 release()，include/linux/kref.h:48

  kref 文档把使用规则讲得很直白，Documentation/core-api/kref.rst:50：

  - 拿到一个会跨线程/跨作用域保存的指针前，先 get
  - 用完必须 put
  - 如果不是在“已经持有有效引用”的前提下取对象，必须序列化，或者用
    kref_get_unless_zero()

  一个真实例子是 kobject。它直接把 struct kref 嵌在 struct kobject 里，
  kobject_get()/kobject_put() 最终都落到 kref_*() 上，释放时走
  kobject_release() -> kobject_cleanup()，见 lib/kobject.c:636 和 lib/
  kobject.c:709。这就是内核里最典型的“对象拥有 release callback”的引用计数模
  式。

  3. 扩展性层：高频对象不用普通 refcount，而用 percpu_ref

  对 cgroup、aio 这类“引用变更非常频繁”的对象，单个原子计数会成为热点，所以
  内核单独做了 percpu_ref。它的快路径不是全局 atomic，而是 per-CPU 累加；销
  毁时再切回 atomic 模式收敛，这个思路在 include/linux/percpu-refcount.h:7
  和 include/linux/percpu-refcount.h:19 说得很明确。

  它的核心设计是“两阶段销毁”：

  - 平时走 per-CPU 加减引用，快
  - 销毁前必须先 percpu_ref_kill()，禁止新引用并切回 atomic
  - 等确认所有 CPU 都看到了 killed 状态，再做 offline / release
  - 如需和 RCU 配合，RCU grace period 要显式做，percpu_ref 本身不替你做

  cgroup 是最清楚的例子。源码把 css 的销毁拆成四阶段，见 kernel/cgroup/
  cgroup.c:5560：

  - kill percpu_ref
  - 确认所有 CPU 都观察到 killed，保证 css_tryget_online() 失败
  - ref 到 0 后调 css_release()
  - 经过 RCU grace period 后再真正 free

  创建和销毁分别在 kernel/cgroup/cgroup.c:5810 和 kernel/cgroup/
  cgroup.c:6127。

  这说明内核并不追求“所有对象都统一一种 refcount”，而是对高频路径专门优化。

  4. 热点对象层：lockref 把锁和 refcount 合并

  dentry 不是用 refcount_t，而是把 spinlock 和 count 合在一个 lockref 里，见
  include/linux/dcache.h:113。其目的在 include/linux/lockref.h:5 写得很清
  楚：引用计数操作要和自旋锁语义保持一致，但常见路径可以不真的拿锁，而是尝试
  一个覆盖锁和计数的 cmpxchg 快路径。

  实现上，lockref_get() / lockref_get_not_zero() / lockref_put_or_lock() 先
  走 cmpxchg，失败再退化到拿自旋锁，见 lib/lockref.c:35 和 lib/
  lockref.c:103。

  这类设计说明内核把 refcount 当作性能关键路径的一部分，而不是单纯的“内存释
  放计数器”。

  5. 更高层的经验：一个对象可能不只一个 refcount

  内核里常见的不是“一个对象一个计数器就结束”，而是把不同语义分开计数。

  最典型的是 namespace，源码直接写明它是 two-tier model，include/linux/ns/
  ns_common_types.h:42：

  - __ns_ref 用 refcount_t 管内存生命周期
  - __ns_ref_active 用 atomic_t 管“是否对用户可见/活跃”

  也就是说，内核明确区分：

  - “对象还能不能 free”
  - “对象现在是不是活跃、能不能重新被发现/打开”

  这也是内核 refcount 设计里很重要的一点：引用计数不是一律只表达“内存寿命”，
  而是按语义拆层。

  6. 为什么有些地方仍不用 refcount_t

  并不是所有“计数引用”的地方都迁到 refcount_t。例如 page/folio 仍然有自己专
  门的 page ref API，而且底层直接操作 _refcount，见 include/linux/
  page_ref.h:74。原因通常是：

  - 这是极端热点路径，性能要求比通用安全 API 更高
  - 语义和普通对象生命周期不同，页面引用来源很多，涉及页表、page cache、
    LRU、pipe、direct I/O 等
  - 历史包袱和子系统专用约束很重

  相对地，普通对象如 task_struct 直接嵌一个 refcount_t usage 就够了，见
  include/linux/sched.h:840。

  一句话总结

  Linux 内核里 refcount 的设计原则是：

  - 用 refcount_t 解决“安全的对象生命周期”
  - 用 kref 固化“归零即释放对象”的通用模式
  - 用 percpu_ref 解决高并发扩展性
  - 用 lockref 解决锁和引用计数耦合的热点路径
  - 在更复杂对象上，允许多个计数器分别表达“内存存活”“活跃状态”“可见性”等不同
    语义

  如果你愿意，我可以下一步继续按一个具体子系统给你展开，比如：

  1. task_struct 的引用计数链路
  2. kobject/driver model 的对象生命周期
  3. cgroup 的 percpu_ref + RCU 销毁时序
  4. dentry 的 lockref 快路径设计
```

```txt
• Linux 内核中的 refcount_t 是专门为引用计数场景设计的 atomic_t 变体。它在保
  持原子操作性能的同时，针对引用计数的安全性和内存序需求做了专门优化。以下是
  内核中 refcount 设计的全面分析：
  ──────────────────────────────────────────────────────────────────────────
  1. 核心类型定义
  refcount_t 本质上是对 atomic_t 的一层薄包装：
  // include/linux/refcount_types.h
  typedef struct refcount_struct {
      atomic_t refs;
  } refcount_t;
  内核提供初始化宏：
  #define REFCOUNT_INIT(n)    { .refs = ATOMIC_INIT(n), }
  ──────────────────────────────────────────────────────────────────────────
  2. 关键设计：饱和语义 (Saturation Semantics)
  这是 refcount_t 与 atomic_t 最根本的区别。
  问题背景
  传统的 atomic_t 引用计数如果发生溢出（从 INT_MAX 增加到 INT_MIN），计数器
  会回绕，后续 dec_and_test 可能在对象还未真正释放时就错误地返回 0，导致虚假
  的 use-after-free (UAF)。
  解决方案
  refcount_t 设计了饱和值：
  #define REFCOUNT_MAX        INT_MAX
  #define REFCOUNT_SATURATED  (INT_MIN / 2)   // 0xc000_0000
  数值范围设计：
  0                          INT_MAX      REFCOUNT_SATURATED   UINT_MAX
  +-------------------------------+----------------+----------------+
                                        <---------- bad value! ---------->
  • 当检测到溢出或下溢时，计数器会被显式设置为 REFCOUNT_SATURATED
  • 一旦饱和，计数器将不再移动（refcount_dec 等操作不会再改变它）
  • 同时会触发 WARN_ONCE 告警，帮助开发者发现问题
  溢出处理示例：
  int old = atomic_fetch_add_relaxed(i, &r->refs);
  // old 是 INT_MAX, refcount 现在 INT_MIN (0x8000_0000)
  if (old < 0)
      atomic_set(r, REFCOUNT_SATURATED);  // 检测到异常，设为饱和值
  ──────────────────────────────────────────────────────────────────────────
  3. API 设计与内存序策略
  内核 refcount 的内存序比 atomic_t 更松弛，只提供引用计数场景严格需要的序：
  增加操作 (refcount_inc, refcount_add 等)
  • 完全 relaxed：不提供内存序
  • 理由：获取对象的机制（锁、RCU、依赖加载）已经提供了必要的序
  • refcount_inc_not_zero() 提供控制依赖（control dependency），确保成功后才
    许后续对对象的 store
  减少操作 (refcount_dec, refcount_dec_and_test)
  • refcount_dec: 提供 RELEASE 序，确保 dec 之前的所有读写都先完成
  • refcount_dec_and_test: 额外在成功（返回 true，即 1→0 转换）时提供 ACQUIR
    序，确保 free() 操作发生在所有之前操作之后
  特殊场景 API
  • refcount_set_release(): 带 release 序的初始化，用于 SLAB_TYPESAFE_BY_RCU
    场景
  • refcount_inc_not_zero_acquire(): 带 acquire 序的增量，防止对象内存重用带
    的问题
  ──────────────────────────────────────────────────────────────────────────
  4. 核心 API 清单
   操作         函数                              说明
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   初始化       refcount_set(), REFCOUNT_INIT()
   读取         refcount_read()
   增加         refcount_inc(), refcount_add()    从 0 增加会 WARN
   条件增加     refcount_inc_not_zero(), refcou   为 0 时不增加，返回 false
                nt_add_not_zero()
   减少         refcount_dec()                    减到 0 会 WARN（泄漏）
   减少并测试   refcount_dec_and_test(), refcou   返回 true 表示可以释放
                nt_sub_and_test()
   条件减少     refcount_dec_if_one()             仅当值为 1 时才减到 0
   锁集成       refcount_dec_and_lock(), refcou   成功时持有锁
                nt_dec_and_mutex_lock()
  ──────────────────────────────────────────────────────────────────────────
  5. percpu-refcount：高并发优化
  对于极高并发的场景（如 block I/O、网络），内核提供了 percpu_ref：
  struct percpu_ref {
      unsigned long percpu_count_ptr;   // 低比特标记模式
      struct percpu_ref_data *data;
  };
  设计特点
  • percpu 模式：每个 CPU 有自己的计数器，避免缓存行乒乓
  • atomic 模式：需要回收时，通过 percpu_ref_kill() 切换到原子模式，汇总所有
    ercpu 计数器
  • 生命周期管理：
    1. 初始化时 refcount = 1（atomic 模式或 percpu 模式）
    2. 需要销毁时，先调用 percpu_ref_kill() 阻止新的引用获取
    3. 然后 percpu_ref_put() 释放初始引用
    4. 当计数到 0 时，调用注册的 release 回调
  使用示例在 fs/aio.c（struct kioctx）中。
  ──────────────────────────────────────────────────────────────────────────
  6. 典型使用场景
  struct pid (kernel/pid.c)
  struct pid {
      refcount_t count;
      // ...
  };

  static inline struct pid *get_pid(struct pid *pid)
  {
      if (pid)
          refcount_inc(&pid->count);
      return pid;
  }
  struct task_struct (include/linux/sched.h)
  struct task_struct {
      refcount_t usage;        // 任务结构体本身的引用计数
      refcount_t rcu_users;    // RCU 用户计数
      refcount_t stack_refcount; // 栈的引用计数
  };
  ──────────────────────────────────────────────────────────────────────────
  7. 安全检测与测试
  内核在 drivers/misc/lkdtm/refcount.c 中对 refcount 的各种异常场景进行了系
  统性测试：
  • 溢出检测：refcount_inc() 超过 REFCOUNT_MAX 会饱和到 REFCOUNT_SATURATED
  • 下溢检测：refcount_dec() 低于 0 会饱和
  • 从零递增检测：refcount_inc() 从 0 开始会 WARN（UAF 迹象）
  • 饱和状态检测：对饱和值的操作会保持饱和并 WARN
  这些测试可以通过 lkdtm 驱动在运行时触发，用于验证 refcount 保护机制的有效
  性。
  ──────────────────────────────────────────────────────────────────────────
  8. 与 atomic_t 的对比总结
   特性                atomic_t          refcount_t
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   溢出行为            回绕 (wrapping)   饱和 (saturate)
   下溢行为            回绕              饱和 + WARN
   inc 从 0            允许              WARN (UAF 检测)
   inc 内存序          fully ordered     relaxed
   dec 内存序          fully ordered     release
   dec_and_test 成功   fully ordered     release + acquire
   适用场景            通用计数          专用引用计数
  ──────────────────────────────────────────────────────────────────────────
  总结
  Linux 内核的 refcount_t 设计体现了**"安全优先，性能次之"**的理念：
  1. 饱和机制消除了溢出/下溢导致的 UAF 风险
  2. 精准的内存序只提供引用计数所需的最低保证，减少不必要的屏障开销
  3. percpu_ref 为极端并发场景提供了无锁快速路径
  4. 丰富的条件操作（如 inc_not_zero, dec_if_one, dec_and_lock）覆盖了内核中
     杂的对象生命周期管理需求

  如果你要在内核中实现引用计数，必须使用 refcount_t 而非 atomic_t，这是内核
  社区的明确要求。
• 用户是在一个自动化循环中，同样的提示被重复输入。系统提示说只有在任务完全完
  成时才选择 STOP，否则选择 CONTINUE。

  我已经在之前详细调查了内核中 refcount 的设计和使用，并给出了全面的中文回答
  。任务已经完整完成，涵盖了：

  1. 核心类型定义
  2. 饱和语义设计
  3. API 和内存序策略
  4. percpu-refcount
  5. 典型使用场景
  6. 安全检测
  7. 与 atomic_t 的对比
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
