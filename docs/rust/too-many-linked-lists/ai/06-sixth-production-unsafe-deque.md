# 第六章 A Production Unsafe Deque（生产级 unsafe 双端队列）

本章是全书终章，目标是从零重写一个生产级质量的 `LinkedList<T>`（unsafe doubly-linked deque），功能对标 `std::collections::LinkedList`，即作者 2015 年的 linked-list crate（支持 stable Rust 上的 Cursor）。涵盖 variance/subtyping、PhantomData、panic safety（exception safety）、Send/Sync、compile_fail doc-test、Cursor（split/splice）等全书最难的主题。

## Layout（内存布局设计）

### 两种候选布局

双向链表每个节点存两个方向的指针：`... <-> (ptr, A, ptr) <-> (ptr, B, ptr) <-> ...`。问题是链表的**两端**怎么表示，有两种正当方案：

1. **traditional（传统方案）**：链表本身在栈上存 `front` 和 `back` 两个指针。缺点是两个端点 = 两倍的 corner case，容易漏处理一个而出严重 bug。
2. **dummy node（哨兵哑节点方案）**：加一个不含数据的哑节点把首尾连成环，每个节点永远有真实的前后指针，消除所有 corner case。作者认为很优雅，但对 Rust 有两个实际致命问题：
   - **问题 1：多一次分配和间接寻址**（空链表也要分配哑节点）。缓解方案要么把 corner case 加回来（惰性分配），要么需要 copy-on-write 静态单例（太黑魔法）、要么把哑节点存栈上（没有 C++ move-constructor 的语言做不到，Pin 也许可以但不搞）。
   - **问题 2：哑节点里存什么 T？** 泛型 T 可能无法构造默认值（比如 `Box`）。方案如每节点存 `Option<T>`（臃肿）或 `MaybeUninit<T>`（恐怖）、或继承式 type punning（BTreeMap 就是这么干的，极危险）。
   
结论：在 Rust 里传统方案胜出，沿用第五章 unsafe queue 的基本设计。

### 初版类型定义

```rust
pub struct LinkedList<T> {
    front: Link<T>,
    back: Link<T>,
    len: usize,
}

type Link<T> = *mut Node<T>;

struct Node<T> {
    front: Link<T>,
    back: Link<T>,
    elem: T,
}
```

这个定义能用，但 variance 不对（见下节）。

## Variance and PhantomData

### unsafe 集合的五座大山

1. Variance（本章要处理）
2. Drop Check（默认行为是安全的，搞它的手段是 unstable 的，很难真正触发问题——作者曾经坚持它很重要，现在认为可以不管）
3. NonNull 优化（比如让 `Option<NonNull>` 走 niche optimization；对已经极度浪费内存的 LinkedList 不值得）
4. isize::MAX 分配规则（与本章无关）
5. Zero-Sized Types（与本章无关）

### Subtyping 与 Variance

Rust 有 subtyping：`&'big T` 是 `&'small T` 的 subtype（活得更久的引用可以用在需要更短生命期的地方）。这使得下面的代码能编译——`take_two` 要求两个参数同类型，编译器把 `&'big u32` 隐式"降级"成 `&'small u32`：

```rust
fn two_refs<'big: 'small, 'small>(
    big: &'big u32,
    small: &'small u32,
) {
    take_two(big, small);
}

fn take_two<T>(_val1: T, _val2: T) { }
```

但把引用包进 `Cell` 就报 `error[E0623]: lifetime mismatch`；包进 `Vec` 却又通过了。原因在于 **variance**：

- 涉及可变引用的 subtyping 不安全（可以用 `mem::swap` 之类造出悬垂指针），所以 `&mut T` 对 T 是 **invariant**；`Cell<T>` 因内部可变性等价于 `&mut T`，也是 invariant。
- 其余大部分是 **covariant**（subtyping 穿透传递）。contravariant 存在但极罕见。
- 集合内部虽然有可写指针，但得益于 ownership 系统，`Vec<T>` 语义上等价于直接持有 T，所以可以安全地协变。

**Variance 怎么确定**：编译器采用"variance by example"——看字段的 variance 抄过来，有分歧时 invariance 胜出（安全优先）。

### 坑：`*mut T` 是 invariant

`*mut T` 唯一的"安全特性"就是 invariant——因为大多数人不知道 variance 的存在，而它很可能被当作 `&mut T` 用，错误地协变会极度危险。所以初版 `type Link<T> = *mut Node<T>` 让整个 `LinkedList<T>` 对 T invariant，这不是我们想要的。

**`NonNull<T>` 的魔法**：NonNull 被设计为协变的，其实现毫无魔法——内部存 `*const T`（协变），在 API 边界来回 cast：

```rust
pub struct NonNull<T> {
    pointer: *const T,
}
```

结论：用 `NonNull`，要可空就包 `Option<NonNull<T>>`。

### PhantomData

```rust
pub struct LinkedList<T> {
    front: Link<T>,
    back: Link<T>,
    len: usize,
    /// We semantically store values of T by-value.
    _boo: PhantomData<T>,
}

type Link<T> = Option<NonNull<Node<T>>>;
```

`PhantomData<T>` 是给编译器的"概念字段"：声明本类型行为上**按值持有 T**（虽然实际通过指针间接持有）。它影响 drop check、auto trait（Send/Sync）推断。作者承认这里也许不严格必要，但只要用了 NonNull/raw pointer 就应该加上以明确意图；std 还因 drop check 覆写（unstable 特性）需要它，作者是"刻在 DNA 里的 cargo-cult"。Node 本身真的存了 T，不需要 PhantomData。

## Basics（new / push_front / pop_front / len）

```rust
pub fn new() -> Self {
    Self { front: None, back: None, len: 0, _boo: PhantomData }
}
```

PhantomData 没有字段，直接写类型名构造。

```rust
pub fn push_front(&mut self, elem: T) {
    unsafe {
        let new = NonNull::new_unchecked(Box::into_raw(Box::new(Node {
            front: None,
            back: None,
            elem,
        })));
        if let Some(old) = self.front {
            (*old.as_ptr()).front = Some(new);
            (*new.as_ptr()).back = Some(old);
        } else {
            self.back = Some(new);
        }
        self.front = Some(new);
        self.len += 1;
    }
}
```

- 节点分配用 `Box::new` + `Box::into_raw` 把所有权转移给链表，后面用 `Box::from_raw` 回收释放——Box 一直"魔法般地"理解节点的所有权。
- **编译错误**：`type NonNull<Node<T>> cannot be dereferenced`——NonNull 没有实现 DerefMut（DerefMut 基于 `&mut`，而 unsafe 代码里不想随便引入 safe reference），必须显式 `.as_ptr()` 拿裸指针再解引用：`(*old.as_ptr()).front = ...`。

```rust
pub fn pop_front(&mut self) -> Option<T> {
    unsafe {
        self.front.map(|node| {
            let boxed_node = Box::from_raw(node.as_ptr());
            let result = boxed_node.elem;
            self.front = boxed_node.back;
            if let Some(new) = self.front {
                (*new.as_ptr()).front = None;
            } else {
                self.back = None;
            }
            self.len -= 1;
            result
            // Box 在此隐式释放，且知道里面已经没有 T
        })
    }
}
```

- 关键：`Box::from_raw` 让 Box 复活，move 出 `elem` 后 Box 析构只释放节点内存而不再析构 T。
- 不再需要前几章的 `Option::take`——`Option<NonNull>` 是 Copy 的（作者在这里埋了个伏笔：真的没问题吗？见 panic safety 节）。

测试 `test_basic_front`：空表 pop、单元素 push/pop、反复 push/pop 检查 len，全过。

## Drop and Panic Safety（本章精华之一）

### 坑：`debug_assert!` 也能制造内存安全 bug

`pop_front` 初版里有调试用的完整性检查 `debug_assert!(self.len == 1);`，放在 `Box::from_raw` 取走 elem、但 `self.back = None` 还没执行**之前**。问题在于 **panic 就是隐式 early return**：此时 `boxed_node` 在栈上，若此处 panic，Box 析构释放节点内存，而 `self.back` 还指向已释放的节点 → use-after-free。它会把"len 维护得不好"这种小错升级成可被利用的内存安全漏洞。

同理 `self.len -= 1` 在 debug 下有下溢检查也会 panic——每个算术操作都是 panic-safety 隐患。它相对安全是因为位于所有 invariant 修复**之后**。

### Exception safety 要点

- panic 默认走 unwinding（每个函数立即返回），析构函数照常运行，且 unwind 可以被 catch——所以 panic 之后代码还会继续跑，unsafe 集合必须在任何可能 panic 的点上处于某种自洽状态。
- **Invariant 纪律**：对外部观察者，集合始终维持某些性质（如"链表中任何可达节点都已分配且初始化"）。实现内部可以**临时**破坏 invariant，只要在被观察到之前修复。`&mut self` 方法是 Rust ownership 的杀手锏：保证排他访问，没人能偷看。
- 典型案例 `Vec::drain`：Drain 迭代器持有 `&mut Vec`，可以砸碎 Vec 的核心 invariant（从中间/前面移走元素），因为没人能观察到；但析构函数不可依赖（`mem::forget`），所以 std 在 drain 开始就把 len 设为 0——若用户泄漏 Drain，留下一个**安全但丢数据**的 Vec。"You leak me? I leak you!"
- 修复策略：多用 `Option::take` 这类"事务性"操作（双向链表冗余编码太多，不完全适用）；或删掉 debug_assert，把完整性检查挪到专门的测试辅助函数；保证**一切可能 panic 的代码都在方法开头或结尾**（把 invariant 当 precondition/postcondition 来维护）。
- 修订后的代码中可能 panic 的点只剩：`Box::new`（OOM）和 len 算术，都在方法首尾，安全。

## Boring Combinatorics（组合爆炸的全部 API）

双向 = 所有操作做两遍。作者精心让 `push_front`/`pop_front` 与 back 版本**逐字镜像**（交换 front/back 即可），并坚持统一使用 "front/back" 术语而非 "prev/next" 来减少出错。

- `push_back` / `pop_back`：front/back 互换的镜像代码。
- `front` / `front_mut` / `back` / `back_mut`：

```rust
pub fn front(&self) -> Option<&T> {
    unsafe {
        self.front.map(|node| &(*node.as_ptr()).elem)
    }
}
```

  作者试过 `?` 运算符（`Some(&(*self.front?.as_ptr()).elem)`），觉得对这种简单代码差别不大，而且上一节刚讲过 early return 很"渗人"，坚持显式 `map`。

- `Drop`：复用 pop 即可

```rust
impl<T> Drop for LinkedList<T> {
    fn drop(&mut self) {
        while let Some(_) = self.pop_front() { }
    }
}
```

- **Iter**：双向链表终于解锁 `DoubleEndedIterator`，生产级还要 `ExactSizeIterator`。

```rust
pub struct Iter<'a, T> {
    front: Link<T>,
    back: Link<T>,
    len: usize,
    _boo: PhantomData<&'a T>,
}

impl<'a, T> Iterator for Iter<'a, T> {
    type Item = &'a T;

    fn next(&mut self) -> Option<Self::Item> {
        // 检查 self.front == self.back 很诱人但不对：
        // 那样最后一个元素永远不会被 yield！
        // 那种技巧只对数组的 "one-past-the-end" 指针有效。
        if self.len > 0 {
            self.front.map(|node| unsafe {
                self.len -= 1;
                self.front = (*node.as_ptr()).back;
                &(*node.as_ptr()).elem
            })
        } else {
            None
        }
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        (self.len, Some(self.len))
    }
}

impl<'a, T> DoubleEndedIterator for Iter<'a, T> {
    fn next_back(&mut self) -> Option<Self::Item> {
        if self.len > 0 {
            self.back.map(|node| unsafe {
                self.len -= 1;
                self.back = (*node.as_ptr()).front;
                &(*node.as_ptr()).elem
            })
        } else {
            None
        }
    }
}

impl<'a, T> ExactSizeIterator for Iter<'a, T> {
    fn len(&self) -> usize { self.len }
}
```

  注意：`next` 的终止条件是 `len > 0` 而非 `front == back`，否则单元素永远取不出来。IterMut 双端迭代看起来 sketchy，实际是 sound 的（next 和 next_back 在 len 耗尽后都返回 None，不会重复 yield 同一元素）。

- **IterMut**：与 Iter 完全相同的代码加 `mut`，关键差异在 `_boo: PhantomData<&'a mut T>`——这让 IterMut 对 T invariant（见 Send/Sync 节）。
- **IntoIter**：包装整个链表，next = `pop_front`，next_back = `pop_back`，还实现 `size_hint`/`len`。
- 三个 `IntoIterator` 实现：`&'a LinkedList<T>`、`&'a mut LinkedList<T>`、`LinkedList<T>`。

## Filling In Random Bits（杂项 trait）

```rust
pub fn is_empty(&self) -> bool { self.len == 0 }

pub fn clear(&mut self) {
    while let Some(_) = self.pop_front() { }
}
```

其余 trait：`Default`、`Clone`（遍历 push_back clone）、`Extend`、`FromIterator`、`Debug`（`f.debug_list().entries(self).finish()`）、`PartialEq`/`Eq`、`PartialOrd`/`Ord`（迭代器逐元素比较）、`Hash`：

```rust
impl<T: Hash> Hash for LinkedList<T> {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.len().hash(state);
        for item in self {
            item.hash(state);
        }
    }
}
```

两个值得注意的坑：

1. **命名空间冲突**：std 现在有名为 `Hash` 和 `Debug` 的宏（derive 宏），没导入 trait 时会得到关于宏的诡异错误而不是"missing trait"。
2. **Hash 必须先 hash len**：否则 `["he", "llo"]` 和 `["hello"]` 会产生相同的 hash 流（prefix collision），容易被恶意构造碰撞攻击。

## Testing（基础测试）

12 个测试全过，且 `MIRIFLAGS="-Zmiri-tag-raw-pointers" cargo miri test` 也全过（一次写对，作者在书里真情流露 😭）。测试要点：

- `test_basic_front` / `test_basic`：空表、单元素、多元素 push/pop，front_mut/back_mut 就地修改。
- `test_iterator` / `test_iterator_double_end` / `test_rev_iter`：iter 正向、`rev()`、`next`/`next_back` 交替（交错消费到中间相遇后两边都返回 None）、每步校验 `size_hint`。
- `test_mut_iter` / `test_iterator_mut_double_end`：IterMut 同样覆盖。
- `test_eq` / `test_ord` / `test_ord_nan`：相等、字典序、以及 NaN 下所有比较都为 false（验证 PartialOrd 语义）。
- `test_debug`：`format!("{:?}", list)` 输出 `"[0, 1, 2, ...]"`。
- `test_hashmap`：把 LinkedList 当 HashMap key 用，验证 Hash/Eq 一致性。

## Send, Sync, and Compile Tests

### Send/Sync

Send/Sync 是无代码的 marker trait（unsafe trait，OIBIT）：Send = 可安全移交另一线程；Sync = 可跨线程共享（`&Self: Send`）。普通集合按道理都应满足。

**验证方法**：写一个永不调用的私有函数，编译通过即性质成立：

```rust
#[allow(dead_code)]
fn assert_properties() {
    fn is_send<T: Send>() {}
    fn is_sync<T: Sync>() {}

    is_send::<LinkedList<i32>>();
    is_sync::<LinkedList<i32>>();
    is_send::<IntoIter<i32>>();
    is_sync::<IntoIter<i32>>();
    is_send::<Iter<i32>>();
    is_sync::<Iter<i32>>();
    is_send::<IterMut<i32>>();
    is_sync::<IterMut<i32>>();
    // ... 还有 covariant 检查：
    fn linked_list_covariant<'a, T>(x: LinkedList<&'static T>) -> LinkedList<&'a T> { x }
    fn iter_covariant<'i, 'a, T>(x: Iter<'i, &'static T>) -> Iter<'i, &'a T> { x }
    fn into_iter_covariant<'a, T>(x: IntoIter<&'static T>) -> IntoIter<&'a T> { x }
}
```

**坑**：编译失败——`` `NonNull<Node<i32>>` cannot be sent between threads safely ``。这是 raw pointer 的第二个安全装置：`*const` 和 `*mut`（及 NonNull）显式 opt out 了 Send/Sync。需要手动 unsafe impl 回来：

```rust
unsafe impl<T: Send> Send for LinkedList<T> {}
unsafe impl<T: Sync> Sync for LinkedList<T> {}
unsafe impl<'a, T: Send> Send for Iter<'a, T> {}
unsafe impl<'a, T: Sync> Sync for Iter<'a, T> {}
unsafe impl<'a, T: Send> Send for IterMut<'a, T> {}
unsafe impl<'a, T: Sync> Sync for IterMut<'a, T> {}
```

注意必须写 `unsafe impl`（它们是 unsafe trait，unsafe 代码如并发库会信任你实现正确）。IntoIter 不需要——它只含 LinkedList，auto-derive 即可（所谓 "opt-out"：默认自动实现，可用 `impl !Send for MyType {}` 显式排除）。

### 用 rustdoc compile_fail 测试 invariance

IterMut 必须**不是**协变的（它"像" `&mut T`）。如何测试"某段代码必须编译失败"？用 doc-test + `compile_fail` 标注：

```rust
    /// ```compile_fail
    /// use linked_list::IterMut;
    ///
    /// fn iter_mut_covariant<'i, 'a, T>(x: IterMut<'i, &'static T>) -> IterMut<'i, &'a T> { x }
    /// ```
    fn iter_mut_invariant() {}
```

- 建议先不加 `compile_fail` 跑一遍，确认它确实**因为正确的原因**失败（E0308 lifetime mismatch）——否则忘了 `use` 也会"测试通过"。
- 可以写 `compile_fail,E0308` 指定错误码，但只在 nightly 生效且不建议依赖（会让编译器改进报错变成 breaking change）。

**实验验证测试有效性**：把 IterMut 的 `_boo` 删掉 → 报 `error[E0392]: parameter 'a is never used`（编译器强制你使用生命周期）；改成 `_boo: PhantomData<&'a T>`（错误的 variance 示例）→ 编译通过，但 compile_fail 测试失败（"Test compiled successfully, but it's marked compile_fail"）→ 测试体系确实能抓住 variance 错误。

## Cursors

### 为什么需要 Cursor

到此为止的 LinkedList 与 std 1.0 持平，但**依然没用**：链表的杀手锏是 O(1) split/splice（以及侵入式结构、lock-free、存 DST），而按索引定位第 k 个元素是 O(k)。解法是让用户有状态地迭代到某位置，然后在该点做 O(1) 修改。

为什么不能用 IterMut？因为 by-ref 迭代器返回的引用生命周期不绑定迭代器本身（可以连续 next 拿到多个元素并同时持有），这带来两条硬约束：

- IterMut 永远不能回退再 yield 同一元素（否则用户能拿到同一元素的两个 `&mut`，违反语言基本规则）；
- 迭代器不能提供修改底层集合的方法（可能使已 yield 的引用失效）。

而这正是想要的能力，所以需要新东西：**Cursor**——像文本编辑器里闪烁的光标，可在序列中来回移动并在该点编辑。

### 设计之争（on 还是 between，ghost 节点）

- 光标可以在元素**之间**或**之上**。2018 年 std 的 RFC 2570 描述为"on"（有 current element 概念），但 std 实际实现的文档却写"between + ghost 非元素节点"——那是从作者 2015 年原型里抄的文档。最终作者决定按 **RFC 设计**实现（cursor 在元素上）。
- std 的术语：操作有 "before"（朝 front）和 "after"（朝 back），移动叫 `move_next`/`move_prev`（避免 `next_back` 越走越靠近 front 的脑裂悖论——`iter().next_back()` 取的是 `back()`，之后每次 next_back 沿 "front" 指针走）。

### CursorMut 结构

```rust
pub struct CursorMut<'a, T> {
    list: &'a mut LinkedList<T>,
    cur: Link<T>,
    index: Option<usize>,
}
```

- 持有 `&mut LinkedList`（所以 Cursor 存活期间对 list 的所有访问都被借用检查器挡住——这就是安全性的根基）。
- `cur == None` 表示在 **ghost** 位置（head/tail 之间的虚拟位置，跨过它绕到另一端）；此时 `index` 也是 None。
- 只有 `cursor_mut()`（从 ghost 出发），砍掉 std 的 `cursor_front`/`cursor_back` 以减少 corner case（std 那俩在空表上行为别扭）。

### move_next / move_prev

四种情况：正常移动、移动到 ghost（index 变 None）、从 ghost 移动到 front/back（index 变 Some(0)/Some(len-1)）、空表 ghost 上不动。

```rust
pub fn move_next(&mut self) {
    if let Some(cur) = self.cur {
        unsafe {
            self.cur = (*cur.as_ptr()).back;
            if self.cur.is_some() {
                *self.index.as_mut().unwrap() += 1;
            } else {
                self.index = None; // 走到 ghost
            }
        }
    } else if !self.list.is_empty() {
        self.cur = self.list.front; // ghost -> front
        self.index = Some(0)
    } else {
        // 空表，ghost 是唯一"元素"，不动
    }
}
```

`move_prev` 镜像：沿 `front` 指针走，ghost 时去 `list.back`、index = `Some(len - 1)`。

### current / peek_next / peek_prev

**极其重要**：这些方法必须以 `&mut self` 借用 cursor，返回的引用绑定这次借用——不能让用户同时拿多个 `&mut`，也不能在持有引用时调用 insert/remove/split/splice。lifetime elision 的默认行为恰好如此。

### split_before / split_after

`split_before`：把 cur **之前**的所有元素切出去作为新链表返回（到 ghost 为止；在 ghost 上则返回整个链表，自己变空）。

四种情况：正常、prev 是 ghost、ghost 上（`mem::replace(self.list, LinkedList::new())`，顺带处理了空表情况）、空表。

```rust
pub fn split_before(&mut self) -> LinkedList<T> {
    if let Some(cur) = self.cur {
        unsafe {
            // 当前状态
            let old_len = self.list.len;
            let old_idx = self.index.unwrap();
            let prev = (*cur.as_ptr()).front;

            // self 将变成什么
            let new_len = old_len - old_idx;
            let new_front = self.cur;
            let new_back = self.list.back;
            let new_idx = Some(0);

            // 返回值将变成什么
            let output_len = old_len - new_len;
            let output_front = self.list.front;
            let output_back = prev;

            // 断开 cur 与 prev 之间的链接
            if let Some(prev) = prev {
                (*cur.as_ptr()).front = None;
                (*prev.as_ptr()).back = None;
            }

            self.list.len = new_len;
            self.list.front = new_front;
            self.list.back = new_back;
            self.index = new_idx;

            LinkedList {
                front: output_front,
                back: output_back,
                len: output_len,
                _boo: PhantomData,
            }
        }
    } else {
        std::mem::replace(self.list, LinkedList::new())
    }
}
```

作者的方法论：把状态变化全部拆成 "当前状态 / self 将变成 / 返回值将变成 / 断链 / 落地" 五步，每步平凡到不会错——"别写 unsafe Rust！" 的真心话。可优化点（如 `(*cur.as_ptr()).front.take()` 合并两次访问、删 noop 赋值）故意不做，优先可论证的正确性。

`split_after` 镜像：`new_len = old_idx + 1`，`new_back = cur`，`new_idx = Some(old_idx)`，返回 cur 之后的部分。

### splice_before / splice_after（corner case 之王）

把整条输入链表嫁接到 cur 之前/之后。分四种情况：输入空（不动）、双方非空（内部接链）、ghost 上（拼到 back/front）、自己空（`mem::swap`）。

**写作过程中抓到的两个真实 bug**：

1. 把 `self.list.len += input.len; input.len = 0;` 提出到分支末尾后，"自己空" 分支里 `*self.list = input` 报 "use of moved value" → 改用 `std::mem::swap(self.list, &mut input)`。
2. 先 `input.front.take()` 下一行又 `input.front.unwrap()`——take 之后已经是 None，必 panic。镜像分支同样犯了这个错。教训：先取 `in_front`/`in_back` 局部变量再接链。

最终版本（splice_before）：

```rust
pub fn splice_before(&mut self, mut input: LinkedList<T>) {
    unsafe {
        // 用 take 取输入的指针而不是 mem::forget，
        // 万一以后有自定义 allocator 之类也需要清理
        if input.is_empty() {
            // 输入空，不动
        } else if let Some(cur) = self.cur {
            // 双方都非空
            let in_front = input.front.take().unwrap();
            let in_back = input.back.take().unwrap();

            if let Some(prev) = (*cur.as_ptr()).front {
                // 一般情况：纯内部接链
                (*prev.as_ptr()).back = Some(in_front);
                (*in_front.as_ptr()).front = Some(prev);
                (*cur.as_ptr()).front = Some(in_back);
                (*in_back.as_ptr()).back = Some(cur);
            } else {
                // 无 prev，拼到 front
                (*cur.as_ptr()).front = Some(in_back);
                (*in_back.as_ptr()).back = Some(cur);
                self.list.front = Some(in_front);
            }
            // 光标索引前移 input.len
            *self.index.as_mut().unwrap() += input.len;
        } else if let Some(back) = self.list.back {
            // 在 ghost 上且非空，拼到 back
            let in_front = input.front.take().unwrap();
            let in_back = input.back.take().unwrap();

            (*back.as_ptr()).back = Some(in_front);
            (*in_front.as_ptr()).front = Some(back);
            self.list.back = Some(in_back);
        } else {
            // 自己空：整个换成输入，光标留在 ghost
            std::mem::swap(self.list, &mut input);
        }

        self.list.len += input.len;
        input.len = 0; // 非必需但礼貌（防止 input drop 重复释放）
        // Input dropped here
    }
}
```

注意 `splice_before` 中 cur 在真实元素上时 index 要 += input.len（因为插到了 cur 前面）；`splice_after` 镜像但 **index 不变**。`insert`/`remove`/`remove_current` 留作读者练习（可用 split/splice 组合实现）。

## Testing Cursors

测试改编自 std 的 cursor 测试（`test_cursor_move_peek`、`test_cursor_mut_insert`）。第一次跑 2 个失败，暴露两个真实 bug：

1. **`peek_next` 在 ghost 上返回 None**（应返回 front 元素）：初版 `self.cur.and_then(|node| (*node.as_ptr()).back).map(...)` 在 `cur == None` 时直接放弃，忘了 ghost 语义下要回退到 `self.list.front`。第一次修复加 `.or_else(|| self.list.front)` 又把"元素上 next 为 None"错误地回退到 front（peek 不该环绕）——错误地返回了 `Some(6)`。**教训：别再 head-empty 地盲链 Option 组合子**，老老实实用 if 分 ghost/非 ghost 两种情况：

```rust
pub fn peek_next(&mut self) -> Option<&mut T> {
    unsafe {
        let next = if let Some(cur) = self.cur {
            (*cur.as_ptr()).back   // 正常：沿 back 指针
        } else {
            self.list.front        // ghost：用链表 front
        };
        next.map(|node| &mut (*node.as_ptr()).elem)
    }
}

pub fn peek_prev(&mut self) -> Option<&mut T> {
    unsafe {
        let prev = if let Some(cur) = self.cur {
            (*cur.as_ptr()).front
        } else {
            self.list.back
        };
        prev.map(|node| &mut (*node.as_ptr()).elem)
    }
}
```

2. **测试本身的状态 bug**：`test_cursor_mut_insert` 里被注释掉的 `remove_current` 段会改变列表状态，后续断言忘了这一点 → 失败的不是实现而是移植的测试。修复：直接构造等价初始状态 `m.extend([1, 8, 2, 3, 4, 5, 6])`。

之后补上 `check_links` 完整性校验（iter 正向收集 vs `iter().rev()` 再反回来比较），`cargo test` 14 过，`cargo miri test`（`-Zmiri-tag-raw-pointers`）全过，doc-test 也过。

## 收尾：clippy 与 fmt

最后一关 `cargo clippy` 抓到 4 个警告：

1. `while let Some(_) = self.pop_front() { }` → `while self.pop_front().is_some()`（循环体为空，无所谓但照改）。
2. 多余的固有方法 `into_iter`（`should_implement_trait`）——`IntoIterator` 在 prelude 里且基本是 lang item，固有版本多余，删掉。
3. 同 1（clear 里）。
4. 手动实现 `PartialEq::ne` 没必要（从 std 抄来的 cargo-cult），删掉。

改完 clippy 干净，`cargo fmt` 收尾。**最终代码约 1200 行**，对应 linked-list crate 的 commit 5b69cc29。

## 本章核心概念速查

- **Variance/subtyping**：`&'big T` 是 `&'small T` 的 subtype；`&mut T`、`Cell<T>`、`*mut T` invariant；`*const T`、`NonNull<T>`、大多数容器 covariant；编译器按字段推导，分歧时 invariant 胜出。
- **NonNull 的技巧**：内部存 `*const T` 换取协变，API 边界 cast 回 `*mut T`。
- **PhantomData**：向编译器声明"概念上按值持有 T"，影响 drop check 与 auto traits；用 raw pointer/NonNull 时该加就加。
- **Panic safety**：panic = 隐式 early return，析构照跑、可被 catch；任何可能 panic 的点集合必须自洽；可能 panic 的代码放方法首尾；`debug_assert!` 和算术下溢检查也是 panic 源；`mem::forget` 使析构不可靠（Vec::drain 设 len=0 的先例）。
- **Send/Sync**：raw pointer（含 NonNull）显式 opt out，需 `unsafe impl` 手动加回；普通不含内部可变性的集合这样做是 sound 的。
- **compile_fail doc-test**：测试"必须编译失败"的性质（如 IterMut invariance）；先无标注跑确认因正确原因失败。
- **Cursor**：yield 的引用绑定 cursor 自身的 `&mut` 借用而非底层链表，因此可以来回移动 + 就地修改；ghost 节点语义处理环绕；split/splice 是链表 O(1) 杀手锏。
- **Miri**：`MIRIFLAGS="-Zmiri-tag-raw-pointers" cargo miri test` 验证 stacked borrows 合规。

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
