# 第四章 A Bad but Safe Doubly-Linked Deque（糟糕但安全的双向双端队列）

本章用 `Rc<RefCell<Node>>` 实现一个**完全 safe** 的双向链表（Deque）。核心结论：safe 不等于好用——RefCell 让 API 设计变成噩梦，Iter/IterMut 最终被迫放弃。本章的核心教训是 "interior mutability 适合写安全的 application，不适合写安全的 library"。

## 内存布局设计

每个节点同时持有指向前驱和后继的指针，List 本身持有 head 和 tail 两个指针，从而在两端都能 O(1) 插入/删除。由于存在多重所有权（前驱和后继都指向同一节点），必须用 `Rc`；由于需要通过共享引用修改节点，必须用 `RefCell` 提供 interior mutability：

```rust
use std::rc::Rc;
use std::cell::RefCell;

pub struct List<T> {
    head: Link<T>,
    tail: Link<T>,
}

type Link<T> = Option<Rc<RefCell<Node<T>>>>;

struct Node<T> {
    elem: T,
    next: Link<T>,
    prev: Link<T>,
}
```

### RefCell 核心概念

- `borrow(&self) -> Ref<'_, T>` 与 `borrow_mut(&self) -> RefMut<'_, T>` 的规则和 `&` / `&mut` 完全一致：`borrow` 可以多次调用，`borrow_mut` 要求独占。
- 区别在于 RefCell 在**运行时**而非编译期检查这些规则，违反规则直接 panic。
- `Ref` / `RefMut` 实现了 `Deref` / `DerefMut`，行为类似 `&T` / `&mut T`；同时它们像借用版的 `Rc`——只要 `Ref` 还活着，RefCell 就保持被借用状态。这是正确性所必需的：Ref drop 时会通知 RefCell 借用结束，否则可以在引用存活期间再拿到 `RefMut`，破坏 Rust 的别名规则。
- `Cell<T>` 只适用于 `Copy` 类型；非 Copy 类型要用 `RefCell<T>`。`RefCell` 是单线程的，多线程共享可变状态应使用 `Mutex<T>`。

### 不变式（invariant）

验证双向链表操作正确性的简单方法：**每个节点必须恰好有两个指针指向它**。中间节点被前驱和后继指向，两端节点还被 List 本身指向。写每个操作时统计引用计数的增减是否平衡。

## Building Up：new 与 push_front

`new` 仍然平凡，另抽出 Node 构造函数（因为套娃太长了）：

```rust
impl<T> Node<T> {
    fn new(elem: T) -> Rc<RefCell<Self>> {
        Rc::new(RefCell::new(Node {
            elem: elem,
            prev: None,
            next: None,
        }))
    }
}

impl<T> List<T> {
    pub fn new() -> Self {
        List { head: None, tail: None }
    }
}
```

`push_front` 需要特殊处理空链表边界情况：普通操作只改 head 或 tail 之一，但从空/到空的转换要同时改两个：

```rust
pub fn push_front(&mut self, elem: T) {
    // new node needs +2 links, everything else should be +0
    let new_head = Node::new(elem);
    match self.head.take() {
        Some(old_head) => {
            old_head.borrow_mut().prev = Some(new_head.clone());
            new_head.borrow_mut().next = Some(old_head);
            self.head = Some(new_head);
        }
        None => {
            self.tail = Some(new_head.clone());
            self.head = Some(new_head);
        }
    }
}
```

### 踩坑 1：E0609 no field `prev` on `Rc<RefCell<Node<T>>>`

初版直接写 `old_head.prev = ...` 报 E0609。之前 `Rc<Node>` 时代 `.` 运算符会自动 deref，但 `RefCell` 挡在中间，`.` 不会自动调用 `borrow_mut()`——必须显式写出 `old_head.borrow_mut().prev`。这是 interior mutability 的基本代价：借用检查从编译期挪到了运行期，语法上也必须显式。

## Breaking Down：pop_front 与 Drop

`pop_front` 是 `push_front` 的镜像逻辑。最终版本：

```rust
pub fn pop_front(&mut self) -> Option<T> {
    // need to take the old head, ensuring it's -2
    self.head.take().map(|old_head| {                         // -1 old
        match old_head.borrow_mut().next.take() {
            Some(new_head) => {                               // -1 new
                new_head.borrow_mut().prev.take();            // -1 old
                self.head = Some(new_head);                   // +1 new
                // total: -2 old, +0 new
            }
            None => {
                // emptying list
                self.tail.take();                             // -1 old
            }
        }
        Rc::try_unwrap(old_head).ok().unwrap().into_inner().elem
    })
}
```

取出 `elem` 这一路连踩三个坑，每个都值得记住：

### 踩坑 2：E0507 cannot move out of borrowed content

`old_head.borrow_mut().elem` 失败。`borrow_mut` 只给出 `&mut Node<T>`，不能从中 move 出值。`Box` 时代被宠坏了（Box 可以 move 出内容）。解法：RefCell 有 `into_inner(self) -> T`，消费 RefCell 返回内部值。

### 踩坑 3：E0507 cannot move out of an `Rc`

`old_head.into_inner()` 失败。RefCell 包在 Rc 里，而 `Rc<T>` 只给共享引用（这正是引用计数的意义——共享）。解法同第三章的 Drop：`Rc::try_unwrap`，当 refcount 为 1 时 move 出内容。

### 踩坑 4：Result::unwrap 要求 error 类型实现 Debug

`Rc::try_unwrap` 返回 `Result<T, Rc<T>>`（`Result` 可视为 `None` 分支带数据的泛化 `Option`，这里 error 数据就是你试图 unwrap 的那个 Rc）。直接 `.unwrap()` 报 E0599：`unwrap` 要求 error 分支可 debug-print，而 `RefCell<T>: Debug` 要求 `T: Debug`，`Node` 没有实现 `Debug`。因为按不变式 `try_unwrap` 必然成功，所以先用 `.ok()` 把 Result 转成 Option（丢弃 error 分支）再 `unwrap`：

```rust
Rc::try_unwrap(old_head).ok().unwrap().into_inner().elem
```

### Drop：Rc 无法处理环

这次 Drop 不只是为了避免递归 drop，而是**不实现就内存泄漏**。"双向链表就是一大串首尾相接的小环"——每个节点和相邻节点互相持有 Rc。drop list 时只有两端节点 refcount 降到 1，其余节点因环永远活着。最简单的实现是复用已经写好的 `pop_front`：

```rust
impl<T> Drop for List<T> {
    fn drop(&mut self) {
        while self.pop_front().is_some() {}
    }
}
```

（其实之前的可变 stack 也可以这么写 Drop。）

## Peeking：Ref 的生命周期噩梦

`peek_front` 暴露了整个设计的根本缺陷。

### 踩坑 5：E0515 cannot return value referencing temporary value

照搬旧代码写 `&node.borrow().elem` 失败。`Ref` 实现 `Deref`，但通过 `Deref` 得到的引用生命周期绑定在 **Ref 本身**上而不是 RefCell 上——Ref 必须和引用活得一样久（否则 Ref drop 后 RefCell 认为借用结束，可以再发 `RefMut`，类型系统就破了）。而 `peek` 一返回，局部变量 `Ref` 就离开作用域。结论：**无法把 RefCell 完全封装在实现细节里**，只能把 `Ref` 泄露到公开 API 中：

```rust
use std::cell::{Ref, RefCell, RefMut};

pub fn peek_front(&self) -> Option<Ref<T>> {
    self.head.as_ref().map(|node| {
        Ref::map(node.borrow(), |node| &node.elem)
    })
}
```

### Ref::map

`node.borrow()` 得到的是 `Ref<Node<T>>`，但 API 想返回 `Ref<T>`。`Ref::map` 解决这个问题——就像 Option 的 map，可以对 Ref 做映射，为被借用数据的某个组成部分造一个新的 Ref：

```rust
map<U, F>(orig: Ref<'b, T>, f: F) -> Ref<'b, U>
    where F: FnOnce(&T) -> &U, U: ?Sized
```

测试时因为 `Ref` 不实现 `PartialEq`，断言要手动 deref：`assert_eq!(&*list.peek_front().unwrap(), &3);`

## Symmetric Junk：对称补全

用文本替换 `tail <-> head`、`next <-> prev`、`front -> back` 复制出 `push_back` / `pop_back` / `peek_back`，再补上 `_mut` 变体（用 `RefMut::map` + `borrow_mut`）：

```rust
pub fn peek_front_mut(&mut self) -> Option<RefMut<T>> {
    self.head.as_ref().map(|node| {
        RefMut::map(node.borrow_mut(), |node| &mut node.elem)
    })
}
```

## Iteration

### IntoIter：唯一轻松的迭代器

包装 List 并转发 `pop_front`。因为 Deque 天然双向，还能实现 `DoubleEndedIterator`——它继承自 `Iterator`，只需新增 `next_back`；语义是迭代器本身变成一个 deque，可以从两端消费直到相遇。实现 `DoubleEndedIterator` 后自动获得 `rev()` 方法：

```rust
pub struct IntoIter<T>(List<T>);

impl<T> Iterator for IntoIter<T> {
    type Item = T;
    fn next(&mut self) -> Option<Self::Item> {
        self.0.pop_front()
    }
}

impl<T> DoubleEndedIterator for IntoIter<T> {
    fn next_back(&mut self) -> Option<T> {
        self.0.pop_back()
    }
}
```

### Iter：三次尝试，全部失败，最终放弃

**尝试 1：存 `Ref<Node>`。** `Iter<'a, T>(Option<Ref<'a, Node<T>>>)`。构造可以，但 `next` 里要在迭代器里存放下一个节点的 `Ref` 同时把当前节点 map 成 `Ref<T>`——报错 E0521 borrowed data escapes outside of closure + E0505：从 `head.borrow()` 得到的 Ref 只允许和 `node_ref` 活得一样久，不能像普通引用那样随意拆分。

**尝试 2：`Ref::map_split`。** 它能把一个 `Ref<T>` 拆成两个 Ref：

```rust
pub fn map_split<U, V, F>(orig: Ref<'b, T>, f: F) -> (Ref<'b, U>, Ref<'b, V>) where
    F: FnOnce(&T) -> (&U, &V), U: ?Sized, V: ?Sized
```

用它拆出 `(&node.next, &node.elem)` 后仍报 E0521：`next.as_ref().map(|head| head.borrow())` 的借用只在闭包体内有效。再用一层 `Ref::map` 修正生命周期后又撞 E0308：`expected Ref<Node>, found Ref<RefCell<Node>>`。根因：**链表里嵌套了多层 RefCell，每深入一个节点就要多持有一重 Ref 借用**；要正确表达需要一个存放所有未归还借用的 Ref 栈。死路一条。

**尝试 3：存 `Rc<Node>`。** `pub struct Iter<T>(Option<Rc<Node<T>>>)`。但这带来两个无解问题：

1. `Iterator::next` 返回什么？`&T` 和 `Ref<T>` 都需要预先声明生命周期，而 Rc 版本的 Iter 没有生命周期参数；任何东西借出来都是借自 iterator 自身。有 `owning_ref` crate 可以把 `Rc<Node>` 映射成 `Rc<T>`，但即便做到这点还有第二个问题：
2. **iterator invalidation（迭代器失效）**。之前的 Iter 借用了 list，天然免疫失效问题；而 Rc 版迭代器完全不借用 list，用户可以在迭代时调用 `push`/`pop`。`push` 没事（视图是子区间，list 在视野外增长），pop 视野外的节点也没事，但 **pop 掉迭代器正指向的节点时，那个节点 refcount > 1，`Rc::try_unwrap` 失败，`.ok().unwrap()` 直接 panic**。这其实是很有趣的性质——确定性 panic 而非悬垂指针——但 API 层面不可接受。

### 结论与对比

作者放弃实现 `Iter` 和 `IterMut`。并指出一个与第三章（persistent stack）有趣的镜像对比：

- persistent stack（Rc）：很难**取得所有权**（try_unwrap 经常失败），但**出借引用**毫无压力；
- 本章 deque（Rc<RefCell>）：**取得所有权**没问题，但**出借引用**极其痛苦。

大部分挣扎其实源于想把 RefCell 封装为实现细节并提供体面的 API；如果不在乎到处传递 Node，一切都能做（甚至可以做出多个并发的、运行时检查互不重叠的 IterMut）。最终结论：**interior mutability 适合写安全的 application，不适合写安全的 library**——这种设计更适合作为永远不外露的内部数据结构。

## 测试要点

三个测试，全部通过：

- `basics`：front/back 两端分别测空表 pop 返回 None、push 三个再 pop、再 push 再 pop、直到耗尽。组合状态空间大，作者承认可能有没覆盖的 case，但代码至少"不明显错误"。
- `peek`：四个 peek（front/back × mut/非 mut）在空表返回 None；push 1,2,3 后 front 是 3、back 是 1。因 `Ref`/`RefMut` 不实现比较，断言写成 `&*list.peek_front().unwrap()` 形式。
- `into_iter`：`next`/`next_back` 交替消费直到两端相遇、两侧都返回 None。

## 本章总评

100% safe 的双向链表做出来了，但代价是：实现痛苦、API 泄露实现细节（`Ref` 出现在公开签名里）、缺失基础操作（无 Iter/IterMut）、还到处带着 Rc/RefCell 的运行时检查开销（虽然这些检查对保证 safety 确实是必要的）。双向链表的 aliasing 和 ownership 关系实在太纠缠。从下一章开始转向 **unsafe** 实现，夺回全部控制权。

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
