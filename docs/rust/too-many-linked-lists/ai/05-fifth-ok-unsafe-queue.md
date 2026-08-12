# 第五章 An Ok Unsafe Queue（还行的 unsafe 队列）

本章实现一个**单向链式队列**（singly-linked queue），是本书的重点章：引入 raw pointer 和 Unsafe Rust，然后被 miri 当场抓住 Undefined Behaviour，进而深入学习 Rust 的内存模型（Stacked Borrows），最后用纯 raw pointer 重写全部代码。

代码位于 `lists/src/fifth.rs`，骨架从第二章的栈（second.rs）演化而来。

## 一、Layout：队列的设计权衡

### 1.1 队列 vs 栈

栈在同一端 push/pop；队列在**另一端** pop。单向链表可以任选一端做"尾部操作"，但无论把 push 还是 pop 移到链表尾部，朴素实现都要 O(n) 走完整条链表。作者强调：**性能保证是接口的一部分**——队列的 push/pop 必须是 O(1)。

### 1.2 尾指针（tail pointer）缓存

解法：多存一个指向链表尾部的指针。注意**只有"翻转 push"可行**：翻转 pop 需要把尾指针向后退一格，单向链表做不到 O(1)；翻转 push 只需把头指针向前进。

最终内存布局：

```rust
pub struct List<T> {
    head: Link<T>,
    tail: *mut Node<T>,
}

type Link<T> = *mut Node<T>;

struct Node<T> {
    elem: T,
    next: Link<T>,
}
```

- `tail` 不拥有节点，只是一个"缓存指针"；真正的 ownership 链仍在 `head -> next -> ...`。
- 用 raw pointer 而非 `Option`：`*mut` 本身就是 nullable 的，包 `Option` 反而失去 null pointer optimization。用 `ptr::null_mut()` 表示 None。

### 1.3 踩坑记录一：Box 版 push 的 use-after-move / double-free

第一版尝试 `tail: Option<Box<Node<T>>>`，在 push 里：

```rust
let old_tail = mem::replace(&mut self.tail, Some(new_tail)); // new_tail 被 move
match old_tail {
    Some(mut old_tail) => {
        old_tail.next = Some(new_tail); // error[E0382]: use of moved value
    }
    ...
}
```

根因：`Box` 不实现 `Copy`，不能同时放两处；更重要的是 Box **拥有**所指内存，若能编译通过，两个 Box 指向同一节点会在 drop 时 double-free；而且按当时的写法 `old_tail` 出作用域会 free 掉仍在链表里的旧尾节点。Rust 的 ownership 在这里实实在在地救了命。

### 1.4 踩坑记录二：`&mut self` 自引用版的"pin"

第二版尝试 `tail: Option<&'a mut Node<T>>`（借自 self 内部）。编译报 `error[E0495]: cannot infer an appropriate lifetime...`，把 `push` 签名改成 `pub fn push(&'a mut self, elem: T)` 居然能编译通过——但测试立刻爆炸：`error[E0499]: cannot borrow 'list' as mutable more than once at a time`，11 个错。

根因：我们犯了 Rust 大忌——**把指向自己的引用存在自己里面**。每段代码单独看都"合法"，但组合起来的语义是：`push`/`pop` 会把 `self` 可变借用 `'a`，而 `'a` 又必须覆盖 list 的整个生命周期，于是第一次 push/pop 之后 list 就被永久"锁死"（等效于把自己 pin 住）。这段是全书理解 `Pin` 最好的引子：async 的 Future 之所以需要 `Pin`，正是因为编译器把局部变量打包成 struct 后可能出现自引用。

另一个隐患是 `pop` 里必须记得：

```rust
if self.head.is_none() {
    self.tail = None;
}
```

忘了这一步，tail 就会指向已被移出链表、立即被 free 的节点——悬垂指针。Rust 用"绕圈子的方式"（借用检查）防住了它。

结论：safe 方案全部失败，也不回 `Rc<RefCell>` 地狱，改用 **raw pointer**。

## 二、Unsafe Rust 与 raw pointer 基础

- Unsafe Rust 是 Safe Rust 的**超集**：语义规则完全一样，只是允许多做几件会引发 UB 的事。任何支持 FFI 的语言本质上都是 unsafe 的；Rust 把 unsafe 显式圈出来。
- raw pointer 分 `*const T` 和 `*mut T`：没有生命周期、没有固有 aliasing 规则、可为 null、可悬垂、可未对齐、可指向未初始化内存、可与整数互转、可转类型。`*const` 只读只是一个 lint，转成 `*mut` 就能绕过（但没有写权限时这么干就是 UB）。
- **只有解引用 raw pointer 才是 unsafe 操作**。赋值、判空（`.is_null()`）都是 safe 的——"你只是在读写一个整数"。Rust 刻意把 unsafe 的表面积切到最小。
- 手动解引用必须放在 `unsafe {}` 块里（`error[E0133]`）。两种选择：整个函数标 `unsafe`（变成 unsafe fn，调用方也要 unsafe），或在函数内部开 `unsafe` 块（对外仍暴露 safe API）。我们选后者。
- **unsafe taint**：一旦模块里用了 `unsafe`，整个模块都被"污染"——unsafe 块的正确性依赖块外（甚至函数外）建立的状态。可控的原因是**私有性（privacy）**：字段全私有，外部无法破坏不变量；只要暴露的 API 组合起来安全，整个抽象对外就是安全的。这和 FFI 是同一个道理。
- 顺手踩的语法坑：`*self.tail.next` 不对，运算符优先级问题，必须写 `(*self.tail).next`。

从引用拿 raw pointer 用 coercion：

```rust
let raw_tail: *mut _ = &mut *new_tail;
```

之所以可以先取出指向 Box 内部的指针再移动 Box，是因为 **Box 的内容地址稳定**。但这并不"safe"：drop 掉 Box 后指针就悬垂。

## 三、第一版 unsafe 实现（编译通过但暗藏 UB）

```rust
pub struct List<T> {
    head: Link<T>,
    tail: *mut Node<T>,
}

type Link<T> = Option<Box<Node<T>>>; // 注意：这里还是 Box！

pub fn push(&mut self, elem: T) {
    let mut new_tail = Box::new(Node { elem, next: None });
    let raw_tail: *mut _ = &mut *new_tail;

    if !self.tail.is_null() {
        unsafe {
            (*self.tail).next = Some(new_tail);
        }
    } else {
        self.head = Some(new_tail);
    }

    self.tail = raw_tail;
}

pub fn pop(&mut self) -> Option<T> {
    self.head.take().map(|head| {
        let head = *head;
        self.head = head.next;
        if self.head.is_none() {
            self.tail = ptr::null_mut();
        }
        head.elem
    })
}
```

`cargo test` 全绿。但这只是"碰巧能跑"——**safety 是有状态的**：`pop` 若忘了清空 tail，本函数不会出事，但后续的 `push` 会往悬垂的 tail 写。

## 四、Miri：运行期 UB 检测器

### 4.1 什么是 miri

[miri](https://github.com/rust-lang/miri) 是 Rust MIR（mid-level intermediate representation）的**实验性解释器**。它能跑 cargo 项目的测试套件并检测多类 UB：

- 越界访问、use-after-free
- 使用未初始化数据
- 违反 intrinsic 前置条件（如 `unreachable_unchecked` 被到达、`copy_nonoverlapping` 范围重叠）
- 未对齐访问
- 违反基本类型不变量（如 `bool` 不是 0/1、非法 enum discriminant）
- （实验性）违反 Stacked Borrows aliasing 规则
- （实验性）数据竞争
- 内存泄漏（执行结束时仍有不可达的分配）

UB 本质上是**运行期**现象，能在编译期发现的编译器早就报错了，所以需要 miri 这种运行期检查。可以把它理解为 ubsan + tsan + 更狠。注意两个方向的局限：miri 通过不代表没有 UB（你的测试得真实触发那条执行路径）；miri 报错也不一定 100% 是真 UB（规则还是实验性的）。但除非你在写编译器，miri 报错就修。

### 4.2 安装与使用

miri 深度绑定 rustc 内部，只有 nightly 有：

```text
rustup +nightly-2022-01-21 component add miri
cargo +nightly-2022-01-21 miri test
```

指定具体日期是因为 miri 有时会跟不上最新 nightly（可用 rustup-components-history 页面查）。首次运行还会引导安装 xargo 和 `rust-src` 组件。`+nightly-...` 语法可用 `rustup override set` 省掉。

### 4.3 miri 抓到我们的 UB

对上面"能跑"的第一版，miri 报：

```text
error: Undefined Behavior: trying to reborrow for Unique at alloc84055,
       but parent tag <209678> does not have an appropriate item in the borrow stack
   --> .../core/src/option.rs:846:18
    = note: inside `Option::<Box<fifth::Node<i32>>>::map` ...
note: inside `fifth::List::<i32>::pop` at src/fifth.rs:31:9
```

即 `pop` 里的 `self.head.take().map(...)` 违反了 **Stacked Borrows**。下一节解释。

## 五、Stacked Borrows（堆叠借用）

### 5.1 动机：pointer aliasing（指针别名）

两个指针指向重叠内存时称它们 **alias**。编译器依赖 aliasing 信息做优化：能不能把某个值"记住"（缓存进寄存器）而不用反复 load？能不能省掉某次 store？前提是确认没有"tiny angry man"（别的指针）在背后偷偷改这块内存。如果编译器的 aliasing 假设错了，程序就会被误编译出随机垃圾。aliasing 只在**至少有一方在写**时才真正要紧。

Safe Rust 天然有完美的 aliasing 信息：`&mut` 定义上无别名，`&` 可别名但不可写。

### 5.2 Safe Stacked Borrows：reborrow 的嵌套

`&mut` 可以 reborrow：

```rust
let mut data = 10;
let ref1 = &mut data;
let ref2 = &mut *ref1;

*ref2 += 2;
*ref1 += 1;   // OK：使用顺序是嵌套的
```

交换顺序就编译错（`error[E0503]`）：reborrow 之后，原指针在新借用存活期间不可用。之所以能 reborrow 又保住 aliasing 信息，是因为**所有 reborrow 都是干净嵌套的**——任意时刻只有一个"live"指针。干净的嵌套结构天然是个**栈**：borrow stack。

规则（简化版）：

- 栈顶的指针是 "live" 的，认为自己独占。
- reborrow 时新指针压栈，成为唯一 live 指针。
- 使用栈中较老的指针时，它上方的全部弹栈，它复活为 live。
- **真正的错误是使用已经被弹出栈的指针**。
- borrowchecker 保证 safe 代码遵守此规则（它从"反方向"表述：报错说 ref1 越序，而非 ref2 失效；配合 NLL 效果等价）。

### 5.3 Unsafe Stacked Borrows：raw pointer 怎么参与

borrowchecker 管不到 raw pointer，需要一套运行期模型让 raw pointer 也能参与。核心思想：**把引用转成 raw pointer 基本等于一次 reborrow**——raw pointer 获得该内存的权限，直到这次"reborrow"过期（比如原引用再次被使用）。

但 `&mut -> *mut -> &mut -> *mut` 之后再访问第一个 `*mut` 该怎么算？这正是规则复杂的地方（stacked borrows 刻意做得宽松，尽量让更多直觉上合理的 unsafe 代码合法）。所以作者的选择是：**用 miri 的 extra-strict 模式做实验、找直觉**：

```text
MIRIFLAGS="-Zmiri-tag-raw-pointers" cargo +nightly-2022-01-21 miri test
```

`-Zmiri-tag-raw-pointers` 会给 raw pointer 也分配 tag，区分不同来源的 raw pointer（比默认模式更严格，也更"简单"，利于建立直觉）。全书之后都按这个严格模式对齐。

### 5.4 实战试探 miri（Testing Stacked Borrows）

**实验 1：基本交错。** `ref1 -> ptr2` 后先 `*ref1 += 1` 再 `*ptr2 += 2`：rustc 放行并输出 13，但 miri（strict）报 UB——使用 ref1 把 ptr2 的 tag 弹栈了。

**实验 2：长链。** `ref1 -> ptr2 -> ref3 -> ptr4`，先访问 ptr2（错误顺序）再按栈序访问 ptr4/ref3/ptr2/ref1：miri 在 `*ptr4 += 4` 处报错（ptr2 已被访问过，ptr4 被弹栈）。**去掉 ptr2 那次提前访问、严格按栈序访问，则 miri 通过**。结论：可以混用引用和 raw pointer，只要严格嵌套。

**实验 3：数组与指针偏移。** `ref1_at_0 -> ptr2_at_0 -> ptr3_at_1 = ptr2_at_0.add(1)` 看似按栈序访问却报 UB。根因：**`&mut data[0]` 只借用了第 0 个元素**，从它派生的指针无权访问第 1 个元素。指针不是纯整数——它关联着一段**内存范围**（permission 依附于 allocation 的某段区域）。

**实验 4：raw pointer 派生 raw pointer 共享同一个 borrow（tag）。** 拷贝、`add(0)`、`add(1).sub(1)` 派生出一堆指向同一位置的指针，随便交错使用都合法——miri 对"ptr -> ptr"非常宽松，因为编译器本来就不会对 raw pointer 的读写做引用级优化。

**实验 5：拆分借用。** `&mut data[0]` 和 `&mut data[1]` 同时存在被 borrowchecker 拒（不追踪下标），用 `split_at_mut` 拆分后各自派生指针、交错访问完全合法。这说明 borrow 结构其实更像**树**而非纯栈（概念上可以理解为栈是按字节粒度维护权限的）。

**实验 6：slice 指针。** `slice.as_mut_ptr()` 得到的指针拥有整个 slice 的权限，可以 `add` 偏移访问任意元素，再转回 `&mut` 也合法。

**实验 7：共享引用。**

- 多个 `&` 共享引用随意交错读都合法——共享引用压栈后，**其上方的一切只有读权限**（SharedReadOnly soup，miri 甚至不再区分各个共享 tag）。
- `&i32 as *mut i32` 直接 cast 是编译错（`error[E0606]`），只能转 `*const`；但双重 cast `sref as *const i32 as *mut i32` 能绕过编译器——**绕得过编译器不代表合法**：miri 照样在写入时报 UB（`no item granting write access`）。共享引用存在期间，下方的可变指针被允许假设内存没变；如果你偷偷写了，它们的缓存假设就崩了。
- 但只要**只读**，即使创建了 `*mut` 也没事。
- 共享引用也遵循弹栈规则：`*ptr2 += 2` 之后再读 `sref3` 会报 `trying to reborrow for SharedReadOnly ... parent tag ... not in borrow stack`。

**实验 8：interior mutability 与 UnsafeCell。**

- `Cell<T>` 内部是 `UnsafeCell<T>`，后者是 `#[lang = "unsafe_cell"]` 的语言项（lang item）——编译器内建魔法。`UnsafeCell` 告诉编译器"这块内存别做任何 aliasing 假设"，`&UnsafeCell<T>` 被指到可变数据不算 UB，这就是 interior mutability 的根基。
- 陷阱：`UnsafeCell::get_mut()` 会返回一个**真正的 `&mut i32`**——编译器必须假设 `&mut i32` 独占（否则一切优化都无从谈起），所以一旦用了 `get_mut`，UnsafeCell 的保护就被"解除"，混用 shared ref 立刻 UB。
- 正确姿势：**指针类型里始终保留 `UnsafeCell`**（用 `.get()` 拿内部 raw pointer，不要经由 `get_mut` 拿 `&mut`），这样共享引用、raw pointer 混用全部合法。
- 细节：`&*mref1` 这类表达式并不真的"读取"——`*` 只是指代那个内存位置（place），`&` 只是把地址记下来；但 `&**` 的第一个 `*` 是真 load。

**实验 9：Box。** `Box` 有特殊标注，对编译器而言**很像 `&mut`**（声明唯一所有权）。`Box -> *mut` 之后再用 `*data`（Box 自己），raw pointer 就被弹栈失效；顺序反过来（先用 raw pointer 再用 Box）则合法。**这正是我们第一版队列 UB 的根因**：tail raw pointer 指向某个 Box 内部，而每次正常访问 Box（`head.take()`、`map`、`next` 赋值等）都可能使该 raw pointer 的"reborrow"失效。

### 5.5 写 unsafe 代码的朴素生存法则

**一旦开始使用 raw pointer，就尽量只用 raw pointer。**

配合 safe 接口的三步法：

1. 方法开头：从输入的引用拿到 raw pointer；
2. 中间：只用 raw pointer 干活；
3. 结尾：如需要再把 raw pointer 转回 safe 指针。

类型的私有字段**全部**用 raw pointer 存放。两个补充警示：(a) safe 指针断言的不只是 aliasing，还有已分配、对齐、大小足够、已初始化等，所以状态暧昧时乱扔引用更危险；(b) 即使全在 raw pointer 世界，也不能跨 allocation 偏移访问——"指针就是整数"是错误观点。

## 六、Layout & Basics 重做：全 raw pointer 版

关键改动：`Link<T>` 从 `Option<Box<Node<T>>>` 改为 `*mut Node<T>`，Box 只作为**分配/释放的临时工具**出现：

- `Box::into_raw(Box::new(...))`：消费 Box 返回 raw pointer（对齐、非空），之后内存的销毁责任归调用者。这正好符合三步法："从 safe 开始，转成 raw，只在最后（要 Drop 时）转回来"。
- `Box::from_raw(ptr)`：把 raw pointer 转回 Box，由 Box 的析构释放内存。pop 和 Drop 时用。
- 也考虑过"节点里同时存 `Option<Box<Node>>` 和 raw pointer"（`real_next` 方案），也许合法但太丑，弃用。
- 既然全是 raw pointer，`unsafe` 块就不再刻意收窄——整个方法体包起来。

最终实现：

```rust
pub fn push(&mut self, elem: T) {
    unsafe {
        let new_tail = Box::into_raw(Box::new(Node {
            elem: elem,
            next: ptr::null_mut(),
        }));

        if !self.tail.is_null() {
            (*self.tail).next = new_tail;
        } else {
            self.head = new_tail;
        }

        self.tail = new_tail;
    }
}

pub fn pop(&mut self) -> Option<T> {
    unsafe {
        if self.head.is_null() {
            None
        } else {
            // 从 raw pointer 复活成 Box，出作用域自动释放
            let head = Box::from_raw(self.head);
            self.head = head.next;

            if self.head.is_null() {
                self.tail = ptr::null_mut();
            }

            Some(head.elem)
        }
    }
}

impl<T> Drop for List<T> {
    fn drop(&mut self) {
        while let Some(_) = self.pop() { }
    }
}
```

注意 `pop` 里 `Box::from_raw` 的必要性：不转回来就内存泄漏（miri 也会抓泄漏）。`Drop` 用反复 pop 实现，简洁且天然复用释放逻辑。

`cargo test` 与 `MIRIFLAGS="-Zmiri-tag-raw-pointers" cargo miri test` 双双全绿。作者的自我提醒值得记住：**没找到 UB 不等于证明了没有 UB**（测试覆盖不到的路径仍可能藏着 UB），只是达到了本书愿意接受的严谨度。

## 七、Extra Junk：迭代器、peek 与 PhantomData 插曲

`peek`/`iter`/`IntoIter` 等逻辑与栈版完全一样（只有改变长度的操作才碰 tail），但要改写成 raw pointer 世界。

### 7.1 Iter/IterMut 的类型设计

最初尝试把迭代器也改成纯 raw pointer：

```rust
pub struct Iter<'a, T> {
    next: *mut Node<T>,
}
```

编译报 `error[E0392]: parameter 'a is never used`——struct 声明了生命周期却没用。编译器建议用 `PhantomData`（零大小标记类型，让类型"表现得像"拥有 `T` / 借用 `'a`，常用于 unsafe 代码中未被使用的生命周期参数）。作者决定**留给第六章双链表**再讲，本章换个思路：

**迭代器里保留 safe 引用是合理的**，因为迭代器有天然的嵌套纪律：创建迭代器 → 用引用遍历 → 丢弃迭代器 → 之后才能 `push`/`pop`。迭代期间的解引用可视为 raw pointer 的 reborrow：

```rust
pub struct Iter<'a, T> {
    next: Option<&'a Node<T>>,
}

pub struct IterMut<'a, T> {
    next: Option<&'a mut Node<T>>,
}

pub fn iter(&self) -> Iter<'_, T> {
    unsafe {
        Iter { next: self.head.as_ref() }
    }
}

pub fn iter_mut(&mut self) -> IterMut<'_, T> {
    unsafe {
        IterMut { next: self.head.as_mut() }
    }
}
```

### 7.2 `ptr::as_ref` / `ptr::as_mut` 的危险性

这两个方法把 raw pointer 升级成 `Option<&T>` / `Option<&mut T>`，作者平时"像躲瘟疫一样"避免它们，原因：

1. 它们重新引入引用，违背"只用 raw pointer"的法则，必须自己保证 aliasing 规则；
2. 签名是 `pub unsafe fn as_mut<'a>(self) -> Option<&'a mut T>`——`'a` 与输入无关，是**unbounded lifetime**，可以假装成任意长（甚至 `'static`）。处理方法是立刻把它放进有界的上下文（通常就是"尽快从函数返回，让函数签名约束它"）。

本章的用法属于极少数合理场景：返回值直接被 `Iter<'_, T>` / `peek` 的签名约束。

### 7.3 迭代器与 peek 实现

```rust
impl<'a, T> Iterator for Iter<'a, T> {
    type Item = &'a T;
    fn next(&mut self) -> Option<Self::Item> {
        unsafe {
            self.next.map(|node| {
                self.next = node.next.as_ref();
                &node.elem
            })
        }
    }
}

impl<'a, T> Iterator for IterMut<'a, T> {
    type Item = &'a mut T;
    fn next(&mut self) -> Option<Self::Item> {
        unsafe {
            self.next.take().map(|node| {
                self.next = node.next.as_mut();
                &mut node.elem
            })
        }
    }
}

pub fn peek(&self) -> Option<&T> {
    unsafe {
        self.head.as_ref().map(|node| &node.elem)
    }
}

pub fn peek_mut(&mut self) -> Option<&mut T> {
    unsafe {
        self.head.as_mut().map(|node| &mut node.elem)
    }
}
```

小坑：peek 初版忘了 `.map(|node| &node.elem)`，直接返回 `Option<&Node<T>>` 导致类型不匹配（`error[E0308]`）。`IterMut::next` 必须用 `take()`（`&mut` 不可复制），`Iter::next` 用 `map` 即可（`&` 可复制）。

`IntoIter` 是栈版原样照抄：

```rust
pub struct IntoIter<T>(List<T>);

impl<T> Iterator for IntoIter<T> {
    type Item = T;
    fn next(&mut self) -> Option<Self::Item> {
        self.0.pop()
    }
}
```

## 八、测试要点

- `basics`：沿用栈的测试框架但翻转预期顺序（FIFO）；**新增尾部环节**：pop 空之后再 push（6, 7）并 pop——专门验证"pop 空后 tail 指针被正确清空"，防止悬垂 tail 被后续 push 写入。
- `into_iter` / `iter` / `iter_mut`：常规顺序遍历 + 耗尽检查。
- `miri_food`：专给 miri 准备的"大杂烩"测试——交错调用 push/pop/peek/peek_mut/iter/iter_mut，最后让 Drop 在自然析构里跑一遍，尽可能多地制造指针混用场景，提高 miri 抓到 UB 的概率。这体现了用 miri 的方法论：**测试要尽量搅拌各 API 的交互**，因为 UB 是运行期、路径相关的。
- 所有 16 个测试在 `cargo test` 和 `MIRIFLAGS="-Zmiri-tag-raw-pointers" cargo miri test` 下全绿。

## 九、本章要点速查

- 队列 = 栈 + tail 指针缓存；单向链表只能"翻转 push"，tail 只随 push/pop 维护。
- 自引用 struct（`&'a mut self` 存进自己）会让值被永久借用锁死——这就是 `Pin` 要解决的问题域。
- raw pointer：只有解引用 unsafe；null 即 None；`*const` 的只读可被 cast 绕过但绕过不代表合法。
- unsafe taint + privacy = safe 抽象的根基；unsafe 块的正确性依赖模块级不变量。
- miri = MIR 解释器，运行期抓 UB（越界/UAF/未初始化/对齐/Stacked Borrows/数据竞争/泄漏）；nightly 独占；`-Zmiri-tag-raw-pointers` 开严格模式。
- Stacked Borrows 简化模型：borrow stack，栈顶独占，访问下层弹栈，用被弹栈的指针 = UB；raw pointer 派生链共享 tag；权限按 allocation 内的范围细分；共享引用压栈后上方全部只读；`UnsafeCell` 豁免 aliasing 假设但 `get_mut()` 会解除豁免；`Box` 视同 `&mut`。
- 生存法则：safe 引用进 → raw pointer 干活 → safe 引用出；私有字段全 raw；`Box::into_raw` / `Box::from_raw` 管理所有权进出。
- 迭代器是"合理混用引用"的特例（天然嵌套纪律）；`as_ref`/`as_mut` 的 unbounded lifetime 必须立刻约束；`PhantomData` 留待第六章。

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
