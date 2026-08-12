# 第二章 An Ok Stack（一个还行的单链表栈）

在第一章 "A Bad Stack" 的基础上改进：去掉手造的 `Link` enum（它其实就是低配版 `Option`），全面使用 `Option`；将元素类型泛型化；新增 `peek` / `peek_mut`；实现全部三种迭代器（`IntoIter` / `Iter` / `IterMut`）。过程中学习 Option 的常用组合子、泛型、生命周期（lifetime）、迭代器 trait，以及为什么安全 Rust 里 IterMut 能"奇迹般地"通过编译。

新模块加入方式为 `lib.rs` 中 `pub mod second;`，代码从 `first.rs` 拷贝过来改。

## 内存布局设计

```rust
pub struct List<T> {
    head: Link<T>,
}

type Link<T> = Option<Box<Node<T>>>;

struct Node<T> {
    elem: T,
    next: Link<T>,
}
```

- 第一章手写的 `enum Link { Empty, More(Box<Node>) }` 实际上就是重新发明了一个很差的 `Option<Box<Node>>`，而且错过了一个 bonus：`Option` 是 `Copy` 的（当其内容 `Copy` 时），且带有一整套好用的方法。本章用类型别名 `type Link<T> = Option<Box<Node<T>>>;` 替换。
- 布局同第一章：`Node` 在堆上（`Box`），`List` 只持有指向头节点的指针。`Option<Box<Node>>` 借助 null-pointer optimization，空表（`None`）不占用额外空间，也不会像第一章的 `mem::replace` 式设计那样产生"垃圾节点"。

## Option 进阶用法

### `take` 替代 `mem::replace`

`mem::replace(&mut option, None)` 是极常见的惯用法，`Option` 直接提供了 `take` 方法。`push`/`pop`/`Drop` 全部改用 `self.head.take()`：

```rust
pub fn push(&mut self, elem: T) {
    let new_node = Box::new(Node {
        elem: elem,
        next: self.head.take(),
    });
    self.head = Some(new_node);
}
```

### `map` 与闭包

`match option { None => None, Some(x) => Some(y) }` 这个模式被抽象为 `map`。闭包（closure）是匿名函数，能捕获外部局部变量。`pop` 用 `map` 改写后非常紧凑：

```rust
pub fn pop(&mut self) -> Option<T> {
    self.head.take().map(|node| {
        self.head = node.next;
        node.elem
    })
}
```

### Drop 不变（但值得复述）

```rust
impl<T> Drop for List<T> {
    fn drop(&mut self) {
        let mut cur_link = self.head.take();
        while let Some(mut boxed_node) = cur_link {
            cur_link = boxed_node.next.take();
        }
    }
}
```

必须迭代释放：否则递归析构长链表会栈溢出（第一章已有解释）。本章沿用这一实现。

## 泛型化（Generics）

把所有类型参数加上 `<T>` 即可——"让代码变得尖尖的"：

- 类型定义改成 `List<T>` / `Link<T>` / `Node<T>` 后，原来的 `impl List` 和 `impl Drop for List` 编译报错（`E0107: wrong number of type arguments: expected 1, found 0`）——泛型类型在 impl 处也必须带参数，且实现本身要对任意 `T` 成立：

```rust
impl<T> List<T> { /* ... */ }
impl<T> Drop for List<T> { /* ... */ }
```

- 亮点：`new()` 完全不用改，因为返回类型写的是 `Self`；构造 `List { head: None }` 时也不用写 `List<T>`，类型由返回位置推断。

## Peek：返回引用引发的错误

### 错误尝试

```rust
pub fn peek(&self) -> Option<&T> {
    self.head.map(|node| {
        &node.elem
    })
}
```

编译错误：

- `E0515: cannot return reference to local data` —— `node` 是函数内拥有的值，返回指向它的引用会悬垂；
- `E0507: cannot move out of borrowed content` —— `map` 按值消费 `self`（即整个 `Option`），但 `self.head` 只有共享借用，不能 move。

根因：之前 `pop` 里能直接 `map`，是因为先 `take` 把 Option **拿出来了**；现在只想看一眼、不能动它。

### 正确做法：`as_ref` / `as_mut`

```rust
impl<T> Option<T> {
    pub fn as_ref(&self) -> Option<&T>;
}
```

`as_ref` 把 `&Option<T>` 降级成 `Option<&T>`，不动原来的值（`as_mut` 对应可变版本）。`.` 运算符自动穿透多余的间接层：

```rust
pub fn peek(&self) -> Option<&T> {
    self.head.as_ref().map(|node| {
        &node.elem
    })
}

pub fn peek_mut(&mut self) -> Option<&mut T> {
    self.head.as_mut().map(|node| {
        &mut node.elem
    })
}
```

### 测试时踩的模式匹配坑

想测试 `peek_mut` 真的可写，第一次写：

```rust
list.peek_mut().map(|&mut value| {
    value = 42
});
```

报 `E0384: cannot assign twice to immutable variable`。根因：闭包参数 `|&mut value|` 不是"声明 value 是可变引用"，而是一个**模式匹配**——意思是"参数是个 `&mut`，请把它指向的值拷贝进 value"。正确写法是 `|value|`（此时 `value: &mut i32`），再 `*value = 42`：

```rust
list.peek_mut().map(|value| {
    *value = 42
});
```

## IntoIter：消费式迭代器

`Iterator` trait 定义：

```rust
pub trait Iterator {
    type Item;
    fn next(&mut self) -> Option<Self::Item>;
}
```

- `type Item` 是**关联类型**（associated type），每个实现各自定义。
- `next` 返回 `Option`，把 `has_next` + `get_next` 两个概念合并为一个调用，避免重复检查。
- 集合应提供三种迭代器：`IntoIter`（产出 `T`）、`Iter`（产出 `&T`）、`IterMut`（产出 `&mut T`）。

`IntoIter` 不需要 List 之外的新能力——反复 `pop` 即可，所以实现为 List 的 newtype 元组结构体（tuple struct）：

```rust
pub struct IntoIter<T>(List<T>);

impl<T> List<T> {
    pub fn into_iter(self) -> IntoIter<T> {
        IntoIter(self)
    }
}

impl<T> Iterator for IntoIter<T> {
    type Item = T;
    fn next(&mut self) -> Option<Self::Item> {
        self.0.pop()   // 元组结构体字段按数字访问
    }
}
```

## Iter：生命周期登场

### 最初尝试与 E0106

```rust
pub struct Iter<T> {
    next: Option<&Node<T>>,
}
```

编译报 `E0106: missing lifetime specifier`。作者示范了用 `rustc --explain E0106` 查错误码文档的技巧。胡乱给一切加上 `<'a>`（包括 `&'a node`）只会得到语法错误——`&'a` 只能出现在**类型签名**位置，不能出现在函数体表达式里。

### 生命周期理论（本书的一次正式讲解）

- GC 语言不需要生命周期；C/C++ 允许随意取栈指针导致两类系统性错误：① 持有指向已离开作用域数据的指针；② 持有的指针所指数据被"改没了"。Rust 用 lifetime 同时解决这两个问题，且 99% 的情况下是透明的。
- **lifetime 就是程序中某段代码区域（region/scope）的名字**。给引用打上 lifetime 标签，是说它必须在整个该区域内有效。整个生命周期系统就是一个约束求解器：尝试为每个引用找出满足所有约束的最小区域，找到则编译通过，否则报"活的不够长"。
- 函数体内一般不谈 lifetime（编译器信息完备，可自行推断）；但在**类型和 API 签名**层面编译器无法全知，必须显式声明不同 lifetime 之间的关系。这也是 Rust 的 borrow check 能逐函数局部完成、报错相对局部的原因（否则就要做全程序分析，错误会极其非局部）。
- **生命周期省略（lifetime elision）**规则：

```rust
fn foo(&A) -> &B;              // = fn foo<'a>(&'a A) -> &'a B;
fn foo(&A, &B, &C);            // = fn foo<'a,'b,'c>(&'a A, &'b B, &'c C);
fn foo(&self, &B, &C) -> &D;   // 输出 lifetime 默认来自 self
```

- `fn foo<'a>(&'a A) -> &'a B` 的实际含义：输入必须至少活得和输出一样久；输出活得越久，输入被借用的时间就被拉得越久。

### Iter 的正确签名

```rust
pub struct Iter<'a, T> {
    next: Option<&'a Node<T>>,
}

impl<T> List<T> {
    pub fn iter<'a>(&'a self) -> Iter<'a, T> {
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

要点：结构体含引用就必须声明 lifetime 参数；`iter` 方法声明新鲜 lifetime `'a` 把 `&self` 的借用与返回的 `Iter` 绑定（Iter 活着期间 List 被借走）；`next(&mut self)` 不需要 `'a`，`&mut self` 的借用每次调用都是独立的。

### 类型错误的连环修复

1. `self.head.map(|node| &node)` → `E0308`：得到 `Option<&Box<Node>>`，要的是 `Option<&Node>`。尝试 `&*node` 解引用。
2. `&*node` → `E0515` + `E0507`：忘了 `as_ref`，`map` 把 `Box` **move 走了**，Box 被 drop，引用悬垂。
3. 加 `as_ref` 后 `map(|node| &*node)` 又报 `E0308`：`as_ref` 多引入了一层间接，闭包里 `node: &Box<Node>`，`&*node` 还是 `&Box<Node>`（需要两个 `*`）。
4. 最终解法：**`Option::as_deref`**（Rust 1.40 起稳定；此前要写 `map(|node| &**node)`），一次完成 `Option<Box<T>> → Option<&T>` 的转换。
5. 备选技巧：turbofish `map::<&Node<T>, _>(|node| &node)`，显式告诉编译器 map 的返回泛型是 `&Node<T>`，从而触发 **deref coercion**（借用检查器保证指针安全的前提下，编译器可自动插入 `*` 做类型转换）。此处仅为展示，并非更优写法。
6. `iter` 签名可省略为 `pub fn iter(&self) -> Iter<T>`（elision），或写 `Iter<'_, T>`（Rust 2018 的"显式省略"标记，提示该结构体含 lifetime）。

## IterMut：安全代码的"魔法"

### 为什么 Iter 容易、IterMut 难

把 `next` 的签名糖衣剥掉：

```rust
fn next<'b>(&'b mut self) -> Option<&'a T>;
```

**输入（`&'b mut self`）与输出（`&'a T`）的 lifetime 之间没有约束**——所以 `next` 可以无条件反复调用，每次产出的 `&'a T` 互相共存。对共享引用这没问题；但 `&mut` 要求独占，直觉上 IterMut 似乎不可能安全实现。结论却是：单链表（以及数组、树）的 IterMut 完全可以用安全代码写出。

### 实现过程与 Copy 的揭示

直接把 Iter 改成 mut 版本，两个错误：

1. `iter_mut(&self)` 用共享引用调 `as_deref_mut` → `E0596`，改为 `&mut self`（纯粹的复制粘贴错误）。
2. `self.next.map(...)` → `E0507: cannot move out of borrowed content`。

第 2 个错误暴露了一个此前被掩盖的事实：Iter 的 `next` 里同样写了 `self.next.map(...)` 却能编译，是因为 **`&` 引用是 `Copy` 的，从而 `Option<&>` 也是 `Copy`**——`map` move 走的是一个按位拷贝，原值还在。而 `&mut` **不是** `Copy`（否则同一内存会有两个 `&mut`，违反独占性），所以必须显式 `take` 把 Option 拿出来：

```rust
impl<'a, T> Iterator for IterMut<'a, T> {
    type Item = &'a mut T;
    fn next(&mut self) -> Option<Self::Item> {
        self.next.take().map(|node| {
            self.next = node.next.as_deref_mut();
            &mut node.elem
        })
    }
}
```

**Copy 知识点**：`Box` 这类管理堆分配的类型不能 Copy（不能两个所有者都去 free）；整数等无语义数据可以按位拷贝，是 `Copy`；所有数值基本类型（i32/u64/bool/f32/char...）都是 Copy；成员全 Copy 的用户类型也可声明为 Copy；Copy 类型 move 后旧值仍可用，甚至可以从引用中 move 出来。共享引用 `&` 是 Copy，可变引用 `&mut` 不是。

### 为什么 IterMut 能安全工作

- `take` 拿走 `Option<&mut>` 后，我们独占这个可变引用，之后无人能再看它；
- Rust 允许把对结构体的可变引用**拆分（shard）到各子字段**——因为字段互不相交，且无法"回到上层"再制造别名。

因此这段代码静态保证了"链表中每个元素最多被可变引用访问一次"，完全安全、零 unsafe。同样的逻辑可用于数组、树的安全 IterMut，甚至可做成 `DoubleEnded`（从两端同时消费）。

## 测试要点

`#[cfg(test)] mod test` 下共 5 个测试：

- `basics`：空表 pop 得 `None`；push 1/2/3 后 pop 顺序 3、2；再 push 4/5 验证链表未损坏；pop 到底再得 `None`（沿用第一章的测试）。
- `peek`：空表 `peek`/`peek_mut` 均为 `None`；push 后 `peek() == Some(&3)`；**关键是用 `peek_mut().map(|value| *value = 42)` 真正写入**，再用 `peek`/`pop` 验证值已变——"如果没人真的 mutate，就没测试过 mutability"。
- `into_iter`：`list.into_iter()` 依次产出 `Some(3), Some(2), Some(1), None`。
- `iter`：依次产出 `Some(&3), Some(&2), Some(&1)`。
- `iter_mut`：依次产出 `Some(&mut 3), Some(&mut 2), Some(&mut 1)`。

## 本章最终 API 一览

- `List<T>::new() -> Self`
- `push(&mut self, elem: T)` / `pop(&mut self) -> Option<T>`
- `peek(&self) -> Option<&T>` / `peek_mut(&mut self) -> Option<&mut T>`
- `into_iter(self) -> IntoIter<T>` / `iter(&self) -> Iter<'_, T>` / `iter_mut(&mut self) -> IterMut<'_, T>`
- `impl Drop for List<T>`（迭代释放防栈溢出）
- `IntoIter<T>(List<T>)`：tuple struct，`next` 即 `self.0.pop()`
- `Iter<'a, T> { next: Option<&'a Node<T>> }`，`next` 用 `as_deref` 前进
- `IterMut<'a, T> { next: Option<&'a mut Node<T>> }`，`next` 必须先 `take` 再 `as_deref_mut` 前进

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
