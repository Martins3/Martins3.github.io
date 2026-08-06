# harzard pointer
<!-- 0c1f14c4-cfe9-4e76-ab72-16f4278a3fdd -->

https://en.cppreference.com/w/cpp/thread 这里介绍的，还没有实现
https://melodiessim.netlify.app/intro-hazard-ptrs/
https://ckf104.github.io/posts/Thread-in-UE/ harzard pointer 是实现 lockfree 的基础
游戏引擎这么复杂的吗?

https://mp.weixin.qq.com/s/is1XID2rSWy3vnd0rmhEJA
https://mp.weixin.qq.com/s/MpJHAM_9tVlUTpBXGCgd5Q

## 0. 核心
它解决的是 lock-free 数据结构里一个非常核心的问题：

**一个线程把节点从数据结构里删除之后，什么时候才能真的 `free()`？**

这个问题不是“并发修改”本身，而是 **并发内存回收（safe memory reclamation）**。

## 1. 最典型的问题：lock-free stack

比如一个 Treiber stack：

```c
struct node {
    struct node *next;
    int value;
};

_Atomic(struct node *) head;
```

pop 大致写成：

```c
node *p = atomic_load(&head);
node *next = p->next;

if (CAS(&head, p, next))
    free(p);
```

乍看没问题，但两个线程同时运行：

```text
CPU0                            CPU1

p = head;   // p = A

                                CAS(head, A, B)
                                free(A);

next = p->next;
       ^^^^^^^
       use-after-free
```

也就是说：

> CPU0 只是“读到了 A”，并不代表 A 还活着。

这就是 lock-free 数据结构中最麻烦的地方之一。

---

## 2. Hazard Pointer 的核心思想

Hazard Pointer 的思路非常直接：

> 在解引用一个共享指针之前，先公开声明：
>
> **“我现在可能要访问这个对象，你不能 free 它。”**

每个线程都有一个或几个 hazard pointer slot。

例如：

```c
_Atomic(void *) hazards[MAX_THREADS];
```

CPU0 想访问节点 `A`：

```text
CPU0:

1. p = head
2. hazards[0] = p
3. 再检查 head 是否还是 p
4. 如果还是，就可以安全使用 p
```

CPU1 删除 `A` 后，不能马上：

```c
free(A);
```

而是先把它放进：

```text
retired list
```

以后检查所有 hazard pointers：

```text
hazard[0]
hazard[1]
hazard[2]
...
```

如果没有任何 hazard pointer 指向 `A`：

```text
A ∉ hazard set
```

才能：

```c
free(A);
```

所以可以把 hazard pointer 理解成：

```text
“这个对象目前有人可能正在摸，暂时别回收。”
```

---

## 3. 最关键的地方：为什么需要“读两次”？

这是理解 Hazard Pointer 最重要的一点。

很多人第一次会写成：

```c
p = atomic_load(&head);
atomic_store(&my_hazard, p);

use(p);
```

这是 **错误的**。

因为中间存在窗口：

```text
CPU0                            CPU1

p = head;      // A

                                head = B
                                scan hazards
                                没看到 A
                                free(A)

hazard = A;

use(A);   // UAF
```

你虽然“发布 hazard pointer”了，但发布得太晚。

因此正确方式是：

```c
do {
    p = atomic_load(&head);

    atomic_store(&my_hazard, p);

} while (p != atomic_load(&head));
```

逻辑是：

```text
read pointer
    ↓
publish hazard
    ↓
re-read pointer
```

只有确认：

```text
共享位置仍然指向同一个对象
```

之后，才允许 dereference。

---

## 4. 为什么第二次检查可以解决？

考虑两种情况。

#### 情况 A：删除发生在 hazard 发布之后

```text
CPU0                      CPU1

p = head(A)
hazard = A
                          remove A
                          scan hazard
                          看见 A
                          不 free
check head
use A
```

安全。

---

#### 情况 B：删除发生在 hazard 发布之前

```text
CPU0                      CPU1

p = head(A)

                          remove A
                          scan hazards
                          free A

hazard = A

check head
```

此时：

```text
head != A
```

所以 CPU0 不允许解引用 `A`，而是重新开始。

注意一个很微妙的地方：

```c
hazard = A;
```

即便这里 A 已经 free 了，本身通常还没问题。

因为你只是保存了一个 **指针值**，没有 dereference。

真正关键是：

```c
p->next
```

必须发生在验证成功之后。

---

## 5. 一个典型 HP pop

大致如下：

```c
node *pop(void)
{
    node *p;
    node *next;

retry:
    p = atomic_load_explicit(&head, memory_order_acquire);

    if (!p)
        return NULL;

    atomic_store_explicit(&my_hazard, p,
                          memory_order_seq_cst);

    if (p != atomic_load_explicit(&head,
                                  memory_order_acquire))
        goto retry;

    /* 到这里之后 p 被 hazard 保护 */
    next = p->next;

    if (!atomic_compare_exchange_weak(&head,
                                      &p,
                                      next))
        goto retry;

    atomic_store(&my_hazard, NULL);

    retire(p);

    return p;
}
```

实际代码在 retry 时还要认真清 hazard，以及 memory ordering 往往可以优化，先看逻辑即可。

---

## 6. “删除”和“回收”必须分开

Hazard Pointer 强迫你区分两个概念：

#### logical removal

从数据结构里移除：

```text
A -> B -> C

变成

A    B -> C
```

例如：

```c
CAS(&head, A, B);
```

此时：

```text
A 已经 unreachable
```

但它未必能 free。

---

#### physical reclamation

真正：

```c
free(A);
```

必须等到：

```text
没有 hazard pointer 指向 A
```

所以通常有一个：

```text
retired list
```

例如：

```text
remove A
  ↓
retire(A)
  ↓
retired = [A, ...]
  ↓
数量达到阈值
  ↓
scan all hazard pointers
  ↓
free 没被保护的 retired nodes
```

这是 HP 的基本生命周期：

```text
reachable
   ↓
removed
   ↓
retired
   ↓
no hazard references
   ↓
free
```

---

## 7. 为什么不每删除一个节点就扫描一次？

因为扫描通常是：

```text
O(number_of_threads × hazard_slots)
```

假设：

```text
100 threads
2 HP/thread
```

每次删除都扫 200 个 atomic pointer，会很贵。

所以一般：

```c
retire(node)
{
    retired_list.push(node);

    if (retired_list.size >= threshold)
        scan();
}
```

批量做：

```text
retired:
A B C D E F G
```

hazard set：

```text
{B, F}
```

那么：

```text
A free
B keep
C free
D free
E free
F keep
G free
```

---

## 8. HP 真正保证的是什么？

Hazard Pointer 不是保护：

```text
对象不会从数据结构删除
```

对象完全可以被别人 remove。

它保护的是：

> **只要我的 hazard pointer 仍然指向这个对象，它就不能被释放。**

例如：

```text
CPU0:

hazard = A
```

CPU1 可以：

```text
unlink(A)
```

但是不能：

```text
free(A)
```

直到 CPU0：

```text
hazard = NULL
```

---

## 9. 和 reference count 有什么区别？

直觉上它们很像，但获取引用时存在本质区别。

refcount 想做：

```c
p = head;
refcount_inc(&p->refcnt);
```

问题是：

```text
CPU0                     CPU1

p = head                 remove p
                         refcount -> 0
                         free(p)

refcount_inc(&p->refcnt)
              ^
              UAF
```

也就是说：

> 你要增加 refcount，本身就必须首先保证对象还活着。

这就是一个 chicken-and-egg 问题。

HP 则避免修改对象本身：

```text
HP 放在线程自己的独立 storage 里。
```

所以：

```c
hazard = p;
```

不需要访问：

```c
p->xxx
```

这非常关键。

---

## 10. 和 RCU 很像，但思路完全不同

你最近在看 RCU，所以可以直接这么对比。

RCU 的思路是：

> 不问“谁在使用这个对象”，而是等一个 **grace period**，保证旧 reader 都退出了。

```text
remove A
    ↓
call_rcu(A)
    ↓
grace period
    ↓
free A
```

Hazard Pointer：

> 每个 reader 明确告诉 writer：
>
> “我当前正在保护 A。”

```text
reader:
hazard = A

writer:
remove A
scan hazards
A 被保护 → 暂时不能 free
```

可以理解成：

```text
RCU:
    reader 表示“我在 read-side critical section”

HP:
    reader 表示“我具体正在用这几个对象”
```

这是非常大的区别。

---

## 11. HP vs RCU

简单对比：

|                 | Hazard Pointer            | RCU                         |
| --------------- | ------------------------- | --------------------------- |
| reader 要做什么 | publish pointer           | enter/exit RCU read section |
| 精确度          | 精确到 object             | 精确到 grace period         |
| reader overhead | atomic store + validation | 通常极低                    |
| reclamation     | scan hazard pointers      | 等 grace period             |
| stalled reader  | 只挡住自己保护的对象      | 可能拖延整个 grace period   |
| 常见领域        | 用户态 lock-free          | Linux kernel                |

例如一个线程挂死：

```text
hazard = A
线程永久 sleep
```

结果只是：

```text
A 永远无法 free
```

而其它对象仍然可以回收。

这也是 HP 一个很有意思的性质：

> 一个 stalled reader 对 reclamation 的影响是局部的。

---

## 12. 和 Epoch Based Reclamation（EBR）区别

EBR 更像轻量版 RCU。

reader：

```text
enter epoch
...
exit epoch
```

writer：

```text
retire A at epoch N
```

只有所有线程都已经越过 epoch N：

```text
free A
```

因此：

```text
EBR:
    谁还处在旧 epoch？

HP:
    谁具体还拿着 A？
```

假设：

```text
thread0 卡住
```

EBR 可能导致：

```text
所有 epoch N retired objects 都不能释放
```

HP 只会导致：

```text
thread0 hazard 指向的那些对象不能释放
```

---

## 13. 为什么叫 Hazard Pointer？

因为它不是：

```text
protected pointer
```

这个名称强调的是对 reclaimer 来说：

```text
hazard = A
```

意味着：

> A 是危险的，目前不能回收。

所以：

```text
hazard set = 当前不能 reclaim 的对象集合
```

---

## 14. 一个链表 traversal 为什么可能需要两个 HP？

比如：

```text
A -> B -> C
```

你现在保护 A：

```c
HP0 = A;
```

准备读：

```c
B = A->next;
```

但如果你准备从 A 移动到 B，不能简单：

```c
HP0 = B;
```

因为需要先确保 B 在你 publish hazard 之前没被删除/reclaim。

常见模式是：

```text
HP0 = current
HP1 = next
```

过程：

```text
protect A

read B = A->next

protect B

revalidate A->next == B

clear HP0

current = B
```

所以：

> 一个算法需要几个 hazard slots，往往由“一次同时必须安全持有几个节点”决定。

这也是 HP API 经常提供：

```text
HP[0]
HP[1]
HP[2]
```

的原因。

---

## 15. Hazard Pointer 和 ABA

HP 对 ABA 也很有帮助，但要小心一句话：

> Hazard Pointer 主要解决 reclamation，它可以消除很多“由内存重用造成的 ABA”，但不是一个通用 ABA 解法。

比如：

```text
head = A
```

CPU0 保存 A。

CPU1：

```text
pop A
free A

malloc()
```

碰巧新节点又分配到地址 A：

```text
head = A
```

CPU0 看：

```text
head == A
```

以为没变。

HP 如果正确保护了旧 A，那么：

```text
旧 A 不能被 free
```

自然不能被 allocator 立即复用成另一个 A。

因此这种：

```text
reclamation-induced ABA
```

就避免了。

但是数据结构如果自己可以：

```text
A -> B -> A
```

逻辑状态真正发生 ABA，HP 本身不一定解决。

这种往往需要：

```text
tagged pointer
version counter
```

等机制。

---

## 16. Memory ordering 是 HP 最容易写错的地方之一

从 abstract algorithm 来看：

```text
reader:

P = load(shared)
store(hazard, P)
if (P != load(shared))
    retry
dereference P
```

reclaimer：

```text
remove(P)
scan hazards
if P not found
    free(P)
```

关键要求是：

> “publish hazard”和“reclaimer scan”之间必须有足够强的排序关系，否则双方可能都看不到对方。

危险执行类似：

```text
reader                        writer

P = shared

                              shared = NULL

hazard = P

                              scan hazard -> NULL

shared check -> P ?           free(P)
```

在弱内存模型上，证明正确 ordering 并不简单。

因此经典 Hazard Pointer 算法对：

```text
hazard publish
```

通常要求相对较强的原子语义。

这里不能简单认为：

```c
store_release(hazard, p)
```

一定就够了。

原因是 release 本质上主要约束：

```text
之前操作
   ↓
release
```

而 HP 最关键的关系里还涉及：

```text
publish hazard
   ↓
随后重新 load shared pointer
```

以及和 reclaimer scan 形成跨线程同步。

所以 HP 的 C/C++ memory model 实现经常比算法伪代码难不少。

---

## 17. 从“所有权”角度理解 HP

我觉得最容易建立直觉的方法是区分：

```text
shared reachability
```

和：

```text
temporary ownership
```

数据结构：

```text
head → A
```

表示：

```text
A 由于数据结构可达，因此活着
```

但当你：

```c
p = head;
```

并不能自动把这个生命期转移给你。

HP 做的事情其实是：

```text
shared ownership:
    head → A

            ↓

temporary reader claim:
    HP → A
```

然后 writer 可以断开：

```text
head -X-> A
```

但因为：

```text
HP → A
```

对象仍然不能释放。

最后：

```text
HP = NULL
```

它才真正失去所有保护。

---

## 18. 一个非常简化的完整模型

假设每个线程只有一个 HP：

```c
_Atomic(void *) hp[NTHREADS];

struct retired {
    void *ptr;
    struct retired *next;
};
```

保护：

```c
void *protect(_Atomic(void *) *src)
{
    void *p;

    do {
        p = atomic_load(src);
        atomic_store(&hp[tid], p);
    } while (p != atomic_load(src));

    return p;
}
```

释放保护：

```c
void unprotect(void)
{
    atomic_store(&hp[tid], NULL);
}
```

删除：

```c
p = ...
unlink(p);
retire(p);
```

scan：

```c
for each retired node r:
    found = false;

    for each thread t:
        if (hp[t] == r) {
            found = true;
            break;
        }

    if (!found)
        free(r);
```

核心就是这么简单。

工程实现的复杂性主要在：

```text
1. memory ordering
2. retired list batching
3. thread registration
4. HP slots 管理
5. scan 的性能
6. thread exit 清理
```

---

## 19. 为什么 Hazard Pointer 很漂亮，但内核里 RCU 更常见？

因为 HP 的 reader fast path 并不算便宜。

一次 pointer dereference 可能变成：

```text
load shared pointer
store hazard pointer
load shared pointer again
branch/retry
load object data
```

尤其：

```text
store hazard pointer
```

是一个共享可见的 atomic store。

而 RCU reader 典型场景下可能接近：

```c
rcu_read_lock();
p = rcu_dereference(ptr);
...read...
rcu_read_unlock();
```

在某些内核配置/架构上，read-side overhead 极低。

如果是：

```text
read-mostly
```

RCU 非常有优势。

HP 更常见于：

```text
portable userspace lock-free library
C/C++ concurrent data structure
```

因为没有内核帮你提供：

```text
scheduler-assisted grace period
```

而 HP 可以纯用户态实现。

---

## 20. 最后抓住这三个核心点

如果只记 Hazard Pointer 的三个东西，我建议记：

#### 1. `load → publish → recheck`

不能：

```text
load → dereference
```

也不能：

```text
load → publish → 直接 dereference
```

必须：

```text
load pointer
publish hazard
revalidate pointer
dereference
```

---

#### 2. removal != reclamation

```text
unlink(node)
```

不等于：

```text
free(node)
```

必须先：

```text
retire(node)
```

等待：

```text
node ∉ hazard set
```

---

#### 3. HP 保护的是“对象生命周期”

它不是：

```text
locking
```

也不是：

```text
阻止别人修改节点
```

而只是：

```text
阻止对象在你使用期间被 reclaim
```

**Hazard Pointer 是 reader 把“我准备访问这个具体对象”公开出来，reclaimer 只有确认没有任何 reader 宣称该对象为 hazard 时，才能真正释放它。**

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
