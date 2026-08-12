# 第七章 A Bunch of Silly Lists（一堆整活链表）

本章是"living document"，收录各种离谱但真实可用的链表，展示它们与 Rust 类型系统的互动。书中实现了两个：The Double Single 和 The Stack Allocated List（另预留 Self-Referential Arena List、GhostCell List 两节未写）。

## 7.1 The Double Singly-Linked List（双单链表）

### 设计动机与内存布局

第四章的双向链表之所以难写，是因为我们假设所有链接都朝同一方向，导致没有任何节点独占拥有另一个节点。换一个思路：把链表从中间劈成两半，一半向左、一半向右，各是一个普通的单链表栈：

```rust
pub struct List<T> {
    left: Stack<T>,
    right: Stack<T>,
}
```

两个 `Stack` 之间就是"当前位置"（finger）。这样所有权完全清晰：每个 `Stack` 独占拥有自己那半边的所有节点，不需要 `Rc`/`RefCell`，更不需要 unsafe。

### Stack 的改造

复制第二章的安全栈实现，但把 `push`/`pop` 拆出内部辅助函数，暴露按节点（而非按值）的操作，这样"走动"时可以整体搬移节点、避免重新分配：

```rust
pub fn push(&mut self, elem: T) {
    let new_node = Box::new(Node { elem, next: None });
    self.push_node(new_node);
}

fn push_node(&mut self, mut node: Box<Node<T>>) {
    node.next = self.head.take();
    self.head = Some(node);
}

pub fn pop(&mut self) -> Option<T> {
    self.pop_node().map(|node| node.elem)
}

fn pop_node(&mut self) -> Option<Box<Node<T>>> {
    self.head.take().map(|mut node| {
        self.head = node.next.take();
        node
    })
}
```

### List 的 API

常规操作全部是对左右栈的直接转发：`push_left`/`push_right`/`pop_left`/`pop_right`/`peek_left`/`peek_right`/`peek_left_mut`/`peek_right_mut`。

核心创新是"走动"操作：从一边弹出整个节点，推到另一边，返回 `bool` 表示是否真的移动了：

```rust
pub fn go_left(&mut self) -> bool {
    self.left.pop_node().map(|node| {
        self.right.push_node(node);
    }).is_some()
}

pub fn go_right(&mut self) -> bool {
    self.right.pop_node().map(|node| {
        self.left.push_node(node);
    }).is_some()
}
```

### 测试要点

测试 `walk_aboot` 用注释标注每步的状态，用 `_` 表示 finger 位置，例如 `[0, 2, 3, _, 4, 1]`。覆盖：左右 push/peek、`while list.go_left() {}` 把 finger 移到最左端后 `pop_left()` 返回 `None`（此时所有元素都在右栈）、混合 push/pop 的序列化遍历、最终两侧都弹空。

### 概念：finger 数据结构

这是一个极端的 *finger* 数据结构：对 finger 附近位置的修改是 O(1)，远处操作的开销与 finger 到目标的距离成正比。对比：`&mut` 引用也能沿链接向下走做临时修改，但 `&mut` 无法往回走（借用规则决定），而 finger 可以来回移动。

## 7.2 The Stack-Allocated Linked List（栈分配链表）

### 设计动机

堆分配不是唯一选择。"在栈上动态分配"的简单做法就是：调用函数、获得新的栈帧。任何递归过程如果把当前步骤状态的指针传给下一步，而这个指针又是状态的一部分，就天然构成一个栈分配的链表。本书用 callback 风格把它写成显式的链表。

### 布局与 push

节点本身（不是指针）就是链表，每个节点持有一个指向前一个节点的共享引用，零堆分配：

```rust
pub struct List<'a, T> {
    pub data: T,
    pub prev: Option<&'a List<'a, T>>,
}
```

唯一的操作是 `push`：接收旧链表的引用、当前节点数据和一个 callback；在函数栈帧上构造新节点，把它的引用传给 callback。callback 的返回值原样返回，所以嵌套 callback 可以逐层向外传值：

```rust
impl<'a, T> List<'a, T> {
    pub fn push<U>(
        prev: Option<&'a List<'a, T>>,
        data: T,
        callback: impl FnOnce(&List<'a, T>) -> U,
    ) -> U {
        let list = List { data, prev };
        callback(&list)
    }
}
```

使用方式是嵌套闭包，每层闭包里 `list` 都只在当前栈帧存活：

```rust
List::push(None, 3, |list| {
    List::push(Some(list), 5, |list| {
        List::push(Some(list), 13, |list| {
            println!("{}", list.data);
        })
    })
})
```

### Iter

常规迭代器，沿 `prev` 引用走：

```rust
pub struct Iter<'a, T> {
    next: Option<&'a List<'a, T>>,
}

impl<'a, T> List<'a, T> {
    pub fn iter(&'a self) -> Iter<'a, T> {
        Iter { next: Some(self) }
    }
}

impl<'a, T> Iterator for Iter<'a, T> {
    type Item = &'a T;

    fn next(&mut self) -> Option<Self::Item> {
        self.next.map(|node| {
            self.next = node.prev;
            &node.data
        })
    }
}
```

### 测试要点

`elegance` 测试：三层嵌套 `push`，每层用 `list.iter().copied().sum::<i32>()` 断言从当前节点到链尾所有元素之和（3、5+3、13+5+3），验证遍历方向和 lifetime 正确。

### 踩坑：换成 `&mut` 后编译失败 —— variance

尝试把 `prev` 改成 `Option<&'a mut List<'a, T>>` 以支持修改数据，结果连简化测试都报 `error[E0521]: borrowed data escapes outside of closure`（"list is a reference that is only valid in the closure body"），无限递归式报错。

根因：**这段代码（共享引用版）能编译，其实是悄悄依赖了 variance**。每个节点里存的是"与自己类型完全相同"的 `List<'a, T>`，即所有节点被声明成同一个 `'a`；但客观上每层节点活在严格嵌套的作用域里，外层节点的 lifetime 比内层长。共享引用下编译器会悄悄"收缩"较长的 lifetime 去匹配内层类型，这是安全的——大 lifetime 是小 lifetime "and more"，忘掉多余部分没问题（类比继承体系中把 Cat 传给期望 Animal 的位置）。

但对 `&mut` 这么做就不安全了。如果允许收缩，内层 callback 可以写出 use-after-free：

```rust
List::push(None, 3, |list| {
    List::push(Some(list), 5, |list| {
        List::push(Some(list), 13, |list| {
            // 所有 lifetime 相同，于是可以让父节点持有指向我自己的可变引用！
            *list.prev.as_mut().unwrap().prev = Some(list);
        })
    })
})
```

问题本质：遗忘细节之所以危险，是因为**别处可能还记着这些细节并要求它们成立**——一旦有 mutation，写入方以为类型被"缩短"了，读取方却还期待原来的类型。继承体系类比：

```rust
let mut my_kitty = Cat;                  // 长 lifetime
let animal: &mut Animal = &mut my_kitty; // 收缩掉它是 Cat 的信息
*animal = Dog;                           // 写入短 lifetime 的值
my_kitty.meow();                         // 会叫的 Dog！use-after-free
```

形式化结论：`&'a mut T` 对 `'a` 是 **covariant**，对 `T` 是 **invariant**——嵌套进另一个引用内部后不允许再收缩 lifetime，因此 `&mut &'big mut T` 不能转成 `&mut &'small mut T`。共享引用 `&'a T` 对 `'a` 和 `T` 都 covariant，所以共享引用版能编译。（趣闻：Java 数组允许这种协变，靠运行时检查抛 `ArrayStoreException` 来防"会叫的狗"。）

### 解决方案：interior mutability

回到共享引用版本，要修改数据就用 `Cell` 包裹数据，向编译器声明"只改数据、不动引用"：

```rust
#[test]
fn cell() {
    use std::cell::Cell;

    List::push(None, Cell::new(3), |list| {
        List::push(Some(list), Cell::new(5), |list| {
            List::push(Some(list), Cell::new(13), |list| {
                // 把链表中每个值乘以 10
                for val in list.iter() {
                    val.set(val.get() * 10)
                }

                let mut vals = list.iter();
                assert_eq!(vals.next().unwrap().get(), 130);
                assert_eq!(vals.next().unwrap().get(), 50);
                assert_eq!(vals.next().unwrap().get(), 30);
                assert_eq!(vals.next(), None);
            })
        })
    })
}
```

测试同时验证了 `Cell::set`/`get` 的就地修改和迭代器穷尽后持续返回 `None`。

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
