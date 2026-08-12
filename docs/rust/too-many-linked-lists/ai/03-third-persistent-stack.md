# 第三章 A Persistent Stack（持久化栈）

本章从单一所有权（`Box`）走向共享所有权，实现一个不可变的持久化单链表——函数式程序员熟悉的结构：可以取 head 或 tail，也可以把某人的 head 接到别人的 tail 上。核心工具是 `Rc` 和 `Arc`。所有 API 都是纯函数式的：不修改原 list，而是返回新 list，共享旧 list 的节点。

## 布局设计（Layout）

持久化 list 的核心特征是 tail 可以被免费共享。典型场景：

```text
list1 = A -> B -> C -> D
list2 = tail(list1) = B -> C -> D
list3 = push(list2, X) = X -> B -> C -> D
```

内存中应当是：

```text
list1 -> A ---+
              v
list2 ------> B -> C -> D
              ^
list3 -> X ---+
```

`B` 被多个 list 共享，因此**不能用 `Box`**：Box 是独占所有权，drop list2 时它会释放 B，而 B 还被 list1/list3 引用，这必然出问题。函数式语言靠 tracing GC 解决；Rust 只有**引用计数（reference counting）**，可视为一种简单的 GC——吞吐一般低于 tracing GC，且遇到环就彻底失效（本章的 list 不可能成环）。

所以节点指针从 `Box<Node<T>>` 换成 `Rc<Node<T>>`：

```rust
use std::rc::Rc;

pub struct List<T> {
    head: Link<T>,
}

type Link<T> = Option<Rc<Node<T>>>;

struct Node<T> {
    elem: T,
    next: Link<T>,
}
```

**坑点 1**：`Rc` 不在 prelude 中，必须显式 `use std::rc::Rc;`，否则报 `E0412: cannot find type Rc in this scope`。

**布局设计的代价**：`Rc` 只允许对内部数据取**共享引用**（`&T`），永远拿不到 `&mut T`，也不能把数据 move 出来（除非引用计数为 1）。所以这种 list 是不可变的：不能原地 push/pop，也不能实现 `IterMut` 或按值取元素的 `IntoIter`。作者调侃"直接把 Box 全局替换成 Rc 就完事了？……不行。"——所有 API 都得按函数式风格重写。

## 基本 API（Basics）

### new

```rust
impl<T> List<T> {
    pub fn new() -> Self {
        List { head: None }
    }
}
```

### prepend（对应旧 push）

`push`/`pop` 这种原地修改的 API 不再成立，换成 `prepend` 和 `tail`：消费 `&self` 和元素，返回一个**新 list**。新节点的 `next` 指向旧 list 的 head——但不能 move（旧 list 还要继续用），解决办法是 `Clone`：对 `Rc` 调用 `clone()` 就是增加引用计数（不复制数据），`Option<Rc<..>>` 的 clone 正好做我们想要的事，甚至不需要 match：

```rust
pub fn prepend(&self, elem: T) -> List<T> {
    List { head: Some(Rc::new(Node {
        elem: elem,
        next: self.head.clone(),
    }))}
}
```

**核心概念 —— Clone trait**：几乎所有类型都实现了 Clone，它提供"只给共享引用、造一个逻辑上独立的副本"的通用方式，类似 C++ 的拷贝构造函数，但**永远不会被隐式调用**。`Rc` 正是用 Clone 作为增加引用计数的手段。

### tail（对应旧 pop）

`tail` 返回去掉首元素后的整个 list，即 clone 第二个节点（若存在）。第一版：

```rust
pub fn tail(&self) -> List<T> {
    List { head: self.head.as_ref().map(|node| node.next.clone()) }
}
```

**坑点 2**：编译报 `E0308: mismatched types`。因为 `node.next.clone()` 本身是 `Option<Rc<Node<T>>>`，再用 `map` 包一层就得到 `Option<Option<Rc<..>>>`，而 `head` 要的是 `Option<Rc<..>>`。`map` 的闭包必须返回普通值 `Y`；当闭包本身要返回 `Option<Y>` 时，应改用 **`and_then`**（经典 Option 模式）：

```rust
pub fn tail(&self) -> List<T> {
    List { head: self.head.as_ref().and_then(|node| node.next.clone()) }
}
```

### head（对应旧 peek）

```rust
pub fn head(&self) -> Option<&T> {
    self.head.as_ref().map(|node| &node.elem)
}
```

### 测试

链式调用不可变 API，每步返回新 list；并验证对空 list 再取 `tail()` 不会 panic：

```rust
#[test]
fn basics() {
    let list = List::new();
    assert_eq!(list.head(), None);

    let list = list.prepend(1).prepend(2).prepend(3);
    assert_eq!(list.head(), Some(&3));

    let list = list.tail();
    assert_eq!(list.head(), Some(&2));

    let list = list.tail();
    assert_eq!(list.head(), Some(&1));

    let list = list.tail();
    assert_eq!(list.head(), None);

    // Make sure empty tail works
    let list = list.tail();
    assert_eq!(list.head(), None);
}
```

### Iter

与可变 list 的 `Iter` 完全相同——共享引用遍历节点，`as_deref()` 把 `Option<&Rc<Node>>` 拍平成 `Option<&Node>`：

```rust
pub struct Iter<'a, T> {
    next: Option<&'a Node<T>>,
}

impl<T> List<T> {
    pub fn iter(&self) -> Iter<'_, T> {
        Iter { next: self.head.as_deref() }
    }
}

impl<'a, T> Iterator for Iter<'a, T> {
    type Item = &'a T;

    fn next(&mut self) -> Option<Self::Item> {
        self.next.map(|node| {
            self.next = node.next.as_deref();
            &node.elem
        })
    }
}
```

测试：

```rust
#[test]
fn iter() {
    let list = List::new().prepend(1).prepend(2).prepend(3);

    let mut iter = list.iter();
    assert_eq!(iter.next(), Some(&3));
    assert_eq!(iter.next(), Some(&2));
    assert_eq!(iter.next(), Some(&1));
}
```

**注意**：无法为这种 list 实现 `IntoIter`（按值）或 `IterMut`（可变）——我们对元素只有共享访问权。

## Drop

可变 list 的递归析构问题这里依然存在：drop 时逐个释放节点，若用默认递归 drop，长 list 会爆栈。虽然不可变 list 情况稍好——一旦遇到仍被别的 list 共享的节点，递归就会停下——但仍需手工处理。旧实现的关键行：

```rust
cur_link = boxed_node.next.take();
```

它**修改了 Box 内部的 Node**，而 `Rc` 只给共享访问，不允许这么做。

**解决思路**：如果我们是最后一个知道这个节点的 list（引用计数为 1），把 Node 从 `Rc` 里 move 出来是完全安全的；而"什么时候停"也正好可以用同一件事判断——**当 move 不出来（即节点仍被共享）时就停止**。`Rc::try_unwrap` 正是干这个的：引用计数为 1 时返回 `Ok(Node)`，否则把 `Rc` 原样放回 `Err`：

```rust
impl<T> Drop for List<T> {
    fn drop(&mut self) {
        let mut head = self.head.take();
        while let Some(node) = head {
            if let Ok(mut node) = Rc::try_unwrap(node) {
                head = node.next.take();
            } else {
                break;
            }
        }
    }
}
```

这样长 list 是迭代释放的，且遇到共享边界（`try_unwrap` 失败）立即 break，剩余节点留给它们的 owner 去释放。

## Arc 与线程安全

不可变数据结构的一个主要用途是跨线程共享数据（shared mutable state 是万恶之源，而这里把 mutable 部分杀掉了）。但基于 `Rc` 的 list **不是线程安全的**：Rc 的引用计数增减不是原子的，两个线程同时 clone 可能只增加一次计数，导致内存提前释放。

修复方法：把所有 `Rc` 全局替换成 `std::sync::Arc`。`Arc` 与 `Rc` 接口完全一致，唯一区别是引用计数用**原子操作**修改（有额外开销，所以 Rust 两种都提供）。

**核心概念 —— Send 与 Sync**：Rust 用两个 trait 把线程安全建模为一等公民，写错线程安全是编译期不可能事件。

- `Send`：可以安全地 **move** 到另一个线程。
- `Sync`：可以安全地在多线程间**共享**（即 `T: Sync` ⟹ `&T: Send`）。
- 这里的 "safe" 指不可能造成 **data race**（注意区别于更一般的 race condition）。

它们是 **marker trait**（没有任何接口，只是其他 API 可以要求的属性），并且**自动派生**：一个类型全部由 Send/Sync 类型组成，它自动就是 Send/Sync（类似 Copy 的派生规则，但连 impl 都不用写）。几乎所有类型都是 Send + Sync：大多数类型完全拥有自己的数据所以 Send；跨线程共享只能走共享引用，共享引用不可变所以 Sync。

**核心概念 —— interior mutability（内部可变性）**：此前遇到的都是 *inherited mutability*（继承可变性）——值的可变性继承自其容器的可变性。内部可变性类型打破这条规则，允许**通过共享引用修改数据**，主要分两类：

- **cells**（如 `Cell`/`RefCell`）：仅限单线程，更便宜。
- **locks**（如 `Mutex`/`RwLock`）：可用于多线程。
- 另有 **atomics**：行为类似锁的原语。

`Rc` 和 `Arc` 的引用计数都用了内部可变性，而且这个计数被所有克隆实例共享：`Rc` 内部用 cell，所以 `Rc<T>` 不是 Send/Sync；`Arc` 内部用 atomic，所以（当 `T: Send + Sync` 时）`Arc<T>` 是 Send/Sync。但注意：**把类型塞进 Arc 并不会魔法般地让它线程安全**——Arc 也只是按组成自动派生 Send/Sync 而已，`Arc<T>` 线程安全的前提是 `T` 本身线程安全。

## 最终代码

完整代码见 `src/third-final.md`（即上文各片段的合集：`Rc` 布局 + `new`/`prepend`/`tail`/`head`/`iter` + 基于 `Rc::try_unwrap` 的迭代 `Drop` + `Iter` + 两个测试）。

## 本章要点回顾

- 持久化 list = 共享 tail + 不可变节点，`Rc<Node<T>>` 替代 `Box<Node<T>>`；代价是永远只有共享访问。
- 函数式 API 风格：`prepend`/`tail` 消费 `&self` 返回新 list，共享靠 `Rc::clone()` 增引用计数。
- Option 组合子：`map` 返回普通值，闭包要返回 `Option` 时用 `and_then`（否则得到嵌套 Option 的类型错误）。
- `Drop` 用 `Rc::try_unwrap` 既解决"无法修改 Rc 内部"又顺便判断共享边界。
- 线程安全换 `Arc`；`Send`/`Sync` 是自动派生的 marker trait，`Rc` 因 cell 型引用计数而非线程安全，`Arc` 用 atomic 计数。

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
