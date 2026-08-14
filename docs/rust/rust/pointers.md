# rust 的 smart pointers
<!-- d35e0a6f-09d4-4cf9-a38b-9e1eaf8402de -->

## 15.5
- [ ] TODO 这一章应该算是终结了

这个回答是一个不错的总结:
https://stackoverflow.com/questions/45674479/need-holistic-explanation-about-rusts-cell-and-reference-counted-types
  - 更加丰富的总结 : ![https://github.com/usagi/rust-memory-container-cs](https://media.githubusercontent.com/media/usagi/rust-memory-container-cs/master/3840x2160/rust-memory-container-cs-3840x2160-dark-back-low-contrast.png)

- [x] 所以单线程中间会出现 Cell 的动态检查不通过的情况吗 ?
  - Rust Book 给出的例子是 : 当连续调用两次 borrow_mut，那么就会出现问题, 必须等到第一个 borrow_mut 的生命周期结束才可以。
  - [ ] 但是我感觉这种操作，为什么需要动态的检查

A common way to use `RefCell<T>` is in combination with `Rc<T>`
一个可以存在多个 owner 同时修改了。

- [ ] 最后使用了 listed list 的例子, 但是无法理解没有了 RefCell 会出现什么问题。

## 然后继续理解下，为什么 loop 的会导致 memory leak

strong 和 weak 的作用就可以了

## 有趣的

```txt
MutexGuard<'a, T>
```

```txt
C++                    Rust

unique_ptr<T>     ≈     Box<T>
shared_ptr<T>     ≈     Rc<T>
shared_ptr<T>     ≈     Arc<T>  // thread safe
weak_ptr<T>       ≈     Weak<T>
```


Box 的本质
```txt
stack                       heap
─────                       ────

p
┌─────────┐
│ ptr ────────────────────>│ 123 │
└─────────┘                └─────┘
```

String 本质
```txt
stack

s
┌────────────┐
│ ptr        │──────┐
│ len = 5    │      │
│ cap = 5    │      │
└────────────┘      │
                    ▼
heap              hello
```

## rust : weak pointr
<!-- 020570eb-4da3-4f3d-a72c-671d955851d4 -->

我想引用这个对象，但不想延长它的生命周期
### 问题
假设两个对象互相强引用：

```rust
struct A {
    b: Rc<B>,
}

struct B {
    a: Rc<A>,
}
```

关系变成：

```text
A ──Rc──> B
^         │
│         │
└──Rc─────┘
```

即使外部已经不再使用 `A` 和 `B`，它们内部的强引用还互相撑着：

```text
A strong_count = 1
B strong_count = 1
```

所以引用计数永远不会降到 0：

```text
A 不能 drop
B 不能 drop
```

最终就是内存泄漏。

`Weak<T>` 的关键点是：

> `Weak` 可以指向对象，但**不增加 strong reference count**。

例如改成：

```text
A ──Rc──> B
^         │
│         │
└─Weak────┘
```

那么外部强引用消失后：

```text
A strong_count -> 0
```

于是 `A` 可以销毁。

随后：

```text
B strong_count -> 0
```

`B` 也可以销毁。

所以 Weak 打破了 ownership cycle。

---

再看一个特别典型的树结构。

父节点拥有子节点，这是自然的：

```text
Parent
  │
  │ Rc
  ▼
Child
```

但子节点通常也希望能访问父节点：

```text
Parent
  │
  ▼
Child
  │
  └────> Parent
```

如果两边都是 `Rc`：

```text
Parent ──Rc──> Child
Parent <──Rc── Child
```

就是引用环。

正确方式通常是：

```text
Parent ──Rc──> Child
Parent <─Weak─ Child
```

语义也非常合理：

> 父节点拥有 child，所以 child 的生命周期由 parent 保证。
> child 只是“知道自己的 parent 是谁”，它不应该因此拥有 parent。

这就是 `Weak` 背后的 ownership 语义。

3. 比如缓存：

```text
Cache
  │
  └── Weak<Object>
```

缓存希望：

> “如果对象还存在，我就复用；如果没人用了，它应该正常销毁。”

如果缓存存的是：

4. 另一个常见场景是 observer/listener。

比如：

```text
Window
 ├── Button
 ├── Label
 └── EventListener
```

listener 想引用 Window，但它不应该：

```text
listener keeps Window alive forever
```

所以这种 back-reference 往往适合 Weak。

### 解决办法
Rust 中通常是：

```rust
use std::rc::{Rc, Weak};

struct Node {
    parent: Weak<Node>,
}
```

但 Weak 有一个重要性质：

```rust
Weak<T>
```

不能直接访问 `T`。

因为 Weak 指向的对象可能已经死了。

你必须：

```rust
weak.upgrade()
```

得到：

```rust
Option<Rc<T>>
```

例如：

```rust
if let Some(parent) = weak_parent.upgrade() {
    println!("parent still alive");
} else {
    println!("parent already dropped");
}
```

为什么是 `Option`？

因为：

```text
Weak
 │
 ▼
object
```

这个 object 不一定还存在。

所以 Weak 的语义其实是：

> “如果这个对象还活着，我可以临时获得一个 strong reference。”

---

可以把 `Rc` 和 `Weak` 理解成两个计数。

假设：

```rust
let a = Rc::new(Foo {});
let w = Rc::downgrade(&a);
```

内部大概：

```text
Rc control block
┌──────────────────┐
│ strong = 1       │
│ weak   = 1       │
│ Foo {...}        │
└──────────────────┘
```

`Rc` drop 后：

```text
strong: 1 → 0
```

此时：

```text
Foo 被 drop
```

但是 control block 还不能马上释放，因为还有：

```text
Weak
```

需要知道对象已经死了。

于是大致变成：

```text
control block
┌──────────────────┐
│ strong = 0       │
│ weak   = 1       │
│ Foo: dead        │
└──────────────────┘
```

此时：

```rust
w.upgrade()
```

得到：

```rust
None
```

等最后一个 Weak 也 drop：

```text
weak = 0
```

control block 才完全释放。

所以可以区分：

```text
strong count
    ↓
决定 T 是否存活

weak count
    ↓
决定 control block 是否还需要存在
```

这个区别非常重要。


### 总结

从 ownership 的角度，三者可以这么理解：

```text
Box<T>
    ↓
唯一 owner

Rc<T> / Arc<T>
    ↓
共享 owner

Weak<T>
    ↓
non-owner observer
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
