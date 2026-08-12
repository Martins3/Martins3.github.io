# 第一章 A Bad Stack（一个糟糕的单链表栈）

本章实现一个最简单的单链表栈（只存 `i32`，非泛型），API 只有 `new` / `push` / `pop` / `Drop`。所谓 "Bad"，是因为这个设计在性能上和通用性上都有缺陷（每元素一次堆分配、不通用），后续章节逐步改进。本章真正的重点是借它引入 Rust 的核心概念：enum、所有权、借用、`Option`、`mem::replace`、测试和手动实现 `Drop`。

代码放在 `src/first.rs`，需要在 `src/lib.rs` 里加 `pub mod first;`。

## 内存布局的演进（本章精华之一）

### 起点：函数式定义的直译——失败

函数式语言对链表的定义是 sum type（Rust 里的 `enum`）：

```rust
pub enum List {
    Empty,
    Elem(i32, List),
}
```

编译报 `error[E0072]: recursive type has infinite size`——递归类型没有 indirection（间接层）时大小无穷。rustc 直接提示插入 `Box`/`Rc`/`&` 等 indirection。

### 第一版：`Box<List>`——能编译但布局糟糕

```rust
pub enum List {
    Empty,
    Elem(i32, Box<List>),
}
```

问题一：尾部浪费。两元素列表的内存布局是：

```text
[] = Stack   () = Heap
[Elem A, ptr] -> (Elem B, ptr) -> (Empty, *junk*)
```

堆上多分配了一个 `Empty` 节点，而且 enum 的内存布局 = tag（区分 variant 的整数）+ 最大 variant 的空间（含对齐）。所以 `Empty` 虽然只有 1 bit 信息，却占用一个指针加一个元素的空间，整个尾部节点全是 junk。

问题二：非均匀分配。第一个元素（栈上的 `List`）不是堆分配的，其余节点在堆上。这破坏了链表的核心优势：**节点在堆上建好后只动指针，永不移动数据**。例如 split 列表：

```text
layout 1（坏）:  [Elem A, ptr] -> (Elem B, ptr) -> (Empty *junk*)
layout 2（好）:  [ptr] -> (Elem A, ptr) -> (Elem B, *null*)
```

layout 2 的 split 只需把 B 的指针拷到栈上再置空；layout 1 还得把 C 从堆里搬到栈上。merge 同理。

### 失败的尝试：`ElemThenEmpty` / `ElemThenNotEmpty`

为避免尾部分配而引入三个 variant（`Empty`、`ElemThenEmpty(i32)`、`ElemThenNotEmpty(i32, Box<List>)`），结果更糟：出现了非法状态 `ElemThenNotEmpty(0, Box(Empty))`，逻辑复杂化，分配依然不均匀。而且这样做反而丢掉了 **null pointer optimization（空指针优化）**。

null pointer optimization：若 enum 形如 `enum Foo { A, B(ContainsANonNullPtr) }`（一个 variant 不含数据、另一个包含永不为 null 的指针），则整个 enum 无需额外 tag——全 0 即 A，否则是 B。这使 `Option<&T>`、`Option<&mut T>`、`Option<Box<T>>`、`Option<Rc<T>>`、`Option<Arc<T>>`、`Option<Vec<T>>` 等**零开销**。这也是 Rust 故意不规定 enum 内存布局的原因。

### 最终布局：分离 List 与 Node

把 "有元素" 和 "分配下一个节点" 两个概念拆开，用 struct 表达 Node，把 `Box` 提到最优位置：

```rust
pub struct List {
    head: Link,
}

enum Link {
    Empty,
    More(Box<Node>),
}

struct Node {
    elem: i32,
    next: Link,
}
```

- 尾部不再有 junk 分配；
- `Link` 正是 null-pointer-optimized 形态（`Option<Box<Node>>` 同构）；
- 所有元素均匀地在堆上分配。
- `List` 是单字段 struct，大小等于 `Link`，零成本抽象。

**坑：private type in public interface（E0446）**。如果让 `pub enum List` 里出现私有的 `Node` 会报错/警告——enum 的 variant 内容是完全公开的，公开接口不能暴露私有类型。解法是改成 `pub struct List` 包裹私有字段 `head`，把 `Link` 和 `Node` 都藏成私有实现细节。

## Rust 核心概念：ownership 与三种 self

方法的 `self` 参数对应三种所有权形式：

- `self`（Value，真所有权）：可按值 move、销毁、修改、借出。按值传参会 move，旧位置失效。大多数方法不该用 `self`（用完 list 就没了）。
- `&mut self`（mutable reference，临时**独占**访问）：可以对值做任何事（包括整体覆盖、swap），唯一禁止的是**不留下替代值就把值 move 走**——否则还给 owner 的是半成品。
- `&self`（shared reference，临时**共享**访问）：一般只能观察不能修改。之所以叫 shared 而非 immutable，是因为 mutation 规则在某些情况（内部可变性）下可以绕过；`&mut` 更准确的名字是 *unique* reference。

## API 实现

### new

`impl` 块关联代码与类型；`new` 是 impl 里的普通函数（Rust 惯例的 "static method"）：

```rust
impl List {
    pub fn new() -> Self {
        List { head: Link::Empty }
    }
}
```

要点：`Self` 是 impl 目标类型的别名；struct 字面量初始化字段；enum variant 用 `::` 命名空间访问；函数最后一个表达式隐式返回（`return` 可提前返回）。

### push——坑：cannot move out of borrowed content

`push` 改变列表，取 `&mut self`。直接写 `next: self.head` 报 `error[E0507]: cannot move out of borrowed content`：通过 `&mut` 只能临时借用 `self`，把字段 move 走会让 `self` 处于部分初始化状态，这正是 `&mut` 唯一不允许的事。

即使先构造新节点再把 `self.head` 赋回去，编译器仍拒绝（原因包括 exception safety——中途 panic 会留下无效状态）。**解法：`mem::replace`，用另一个值"偷梁换柱"地取出借用内容里的值**：

```rust
pub fn push(&mut self, elem: i32) {
    let new_node = Box::new(Node {
        elem: elem,
        next: mem::replace(&mut self.head, Link::Empty),
    });

    self.head = Link::More(new_node);
}
```

先把 `self.head` 临时换成 `Link::Empty` 取出旧值，再装上新头。`mem::replace` 是全书的常客。

### pop——Option、match 与 by-value 匹配

`pop` 返回 `Option<i32>` 处理空表。`Option<T>` 是泛型 enum（`Some(T)` / `None`），被 prelude 隐式导入。判断 `Link` 用 `match` pattern matching。

踩坑序列：

1. 函数体暂不返回值报 `mismatched types`（期望 `Option<i32>` 得到 `()`）；先用 `unimplemented!()`（会 panic 的宏）占位。无条件 panic 属于 diverging function（返回 never type `!`），可用于任何期望值的上下文。
2. `match self.head` 报 `cannot move out of borrowed content`：**match 默认把匹配内容 move 进分支**，而 `self` 只有 `&mut`。rustc 建议 `match &self.head`。
3. 借用后 `self.head = node.next` 又报同样的错——从共享引用里 move 不行。

关键洞察：pop 要**移除**元素，必须拿到 head 的 by-value 所有权；手里只有 `&mut self`，唯一取值的手段还是 `mem::replace` 的 "Empty dance"：

```rust
pub fn pop(&mut self) -> Option<i32> {
    match mem::replace(&mut self.head, Link::Empty) {
        Link::Empty => None,
        Link::More(node) => {
            self.head = node.next;
            Some(node.elem)
        }
    }
}
```

惯用法细节：每个 block 也以最后一个表达式为值；加分号则 block 变成 `()`（无返回值函数如 `push` 实际返回 `()`）。单表达式分支可以省略花括号。

## 测试

- 测试与代码同文件，放在内联子模块 `mod test` 里，函数加 `#[test]`，用 `cargo test` 运行。
- 断言用 `assert_eq!`（不相等就 panic，测试框架靠 panic 判定失败）。
- **坑**：子模块是独立命名空间，需 `use super::List;`。
- 加 `#[cfg(test)]` 让测试模块只在测试时编译（否则正常 build 会警告 unused import）。

```rust
#[cfg(test)]
mod test {
    use super::List;

    #[test]
    fn basics() {
        let mut list = List::new();
        assert_eq!(list.pop(), None);

        list.push(1);
        list.push(2);
        list.push(3);

        assert_eq!(list.pop(), Some(3));
        assert_eq!(list.pop(), Some(2));

        list.push(4);
        list.push(5);

        assert_eq!(list.pop(), Some(5));
        assert_eq!(list.pop(), Some(4));

        assert_eq!(list.pop(), Some(1));
        assert_eq!(list.pop(), None);
    }
}
```

覆盖：空表 pop、LIFO 顺序、pop 后再 push 不损坏、pop 到枯竭。

## Drop——为什么不能依赖默认递归析构

Rust 像 C++ 一样靠析构函数自动清理资源，析构通过实现 `Drop` trait（trait 即 Rust 的接口）：

```rust
pub trait Drop {
    fn drop(&mut self);
}
```

理论上 `List` 不用自己实现 `Drop`——字段的析构会自动级联。但链表的自动级联是递归的：drop A → drop B → drop C……**长链表会爆栈**。

而且它不是尾递归，无法被优化掉。假装是编译器手写一遍就明白了：

```rust
impl Drop for Box<Node> {
    fn drop(&mut self) {
        self.ptr.drop();  // 先析构内容
        deallocate(self.ptr); // 再释放内存 —— 析构调用之后还有活儿，不是尾递归！
    }
}
```

`Box` 必须先析构内容再释放内存（顺序不能反），所以链式 drop 必然层层压栈。

**解法：手动写迭代版 Drop**，把节点逐个从 Box 里掏出来：

```rust
impl Drop for List {
    fn drop(&mut self) {
        let mut cur_link = mem::replace(&mut self.head, Link::Empty);
        // `while let` == "do this thing until this pattern doesn't match"
        while let Link::More(mut boxed_node) = cur_link {
            cur_link = mem::replace(&mut boxed_node.next, Link::Empty);
            // boxed_node 在此出作用域被 drop，
            // 但它的 next 已被置为 Link::Empty，不会触发无界递归。
        }
    }
}
```

每个节点的析构退化为 drop 一个 `next == Empty` 的 Box，递归深度恒为 1。

### Bonus：为什么不用 `while let Some(_) = self.pop() {}`

`pop` 返回 `Option<i32>`，会把节点里的**元素值**搬出来移动；迭代版 Drop 只搬运 `Box<Node>` 指针，`Box` 能就地（in-place）析构内容。将来把链表泛型化后，若元素是"很大的类型且带 Drop impl"，pop 版会昂贵地逐个移动大对象，迭代版不会。想两全的话可抽一个 `fn pop_node(&mut self) -> Link`，让 `pop` 和 `drop` 都基于它实现。

## 最终代码规模

完整代码约 80 行（其中一半是测试），见 `lists/src/first.rs` / 书中 `first-final.md`，即上文各片段的组合：`List`/`Link`/`Node` 三个类型 + `new`/`push`/`pop` + 迭代 `Drop` + `basics` 测试。

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
