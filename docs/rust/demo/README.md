## 基本执行操作
进入到 ~/data/vn/code/src/rust/src/demo 中执行:
```sh
cargo run -- 1
cargo run -- 10
```

运行测试:

To run all tests in this demo project:
```bash
cargo test
```

To run specific tests:
```bash
cargo test <test_name>
```

或者:
cargo test --manifest-path=/home/martins3/data/vn/code/src/rust/src/demo/Cargo.toml

基本的原则是，让 deck 和 src/main.rs 可以快速的联系到一起，
如果回忆不起来，那么就立刻去测试

## std::backtrace
<!-- 5d98e584-d71e-445f-bf70-838d452c6427 -->

参考: https://doc.rust-lang.org/std/backtrace/index.html

```bash
# 基本用法 - 需要设置环境变量才能看到 backtrace
RUST_BACKTRACE=1 cargo run -- 12

# 显示完整 backtrace
RUST_BACKTRACE=full cargo run -- 12
```

主要 API:
- `Backtrace::capture()` - 捕获当前调用栈（受 `RUST_BACKTRACE` 环境变量控制）
- `Backtrace::force_capture()` - 强制捕获，忽略环境变量设置
- `Backtrace::status()` - 获取 backtrace 状态（Captured/Disabled/Unsupported）

代码示例见: `src/backtrace_demo.rs`

## std::future::Future
<!-- 8187dbb1-742f-4075-a7dd-31904c916242 -->

运行:
```bash
cargo run -- 13
```

这个示例用标准库演示 Future 的核心机制:
- `Future::poll()` 不直接阻塞线程，而是返回 `Poll::Pending` 或 `Poll::Ready(value)`
- Future 返回 `Pending` 时，需要通过 `Waker` 通知 executor 之后再 poll
- `async fn` 返回的是一个实现了 `Future` 的状态机，`.await` 会在内部 poll 被等待的 Future
- `Pin` 用来保证 Future 被 poll 之后不会被随意移动，这是 async 状态机安全保存自引用状态的基础

代码示例见: `src/future_demo.rs`

(2026-05-20 ，还没来得及看，但是的确是存在这个东西了)

## rust 中的模块使用
<!-- 82029eb0-69cd-4cd5-9148-bfe648e5ade3 -->

### mod
- pub mod vm; 定义一个公有模块，可以被父模块或 crate 外部通过路径访问
- mod vm; 定义一个私有模块只能在当前模块及其子模块中使用

### use crate
- [ ] 为什么要使用 use crate::addr 而不是 use addr; 如果 addr.rs 和本文件在同一级目录中

https://stackoverflow.com/questions/58034305/cant-import-a-file-into-another-file-in-rust

在文件中继续使用
```txt
mod tests {

}
```

或者
```txt
pub mod tets
{

}
```

是什么意思?

似乎之前都是文件中写的函数就是一个模块，那么这岂不是模块中也有模块了。

## rust 常用的库
<!-- 5dcb594f-835c-4785-9a04-aa9c853f6a89 -->

- serde / serde_derive：做 序列化与反序列化
- clap (command line argument parser) / clap_derive：做 命令行参数解析

你看到的这种 “库 + derive 库” 是 Rust 生态中非常典型的设计：
原因是：
- proc-macro 必须是独立 crate
- 避免强制所有用户依赖宏（减小编译负担）
- 功能拆分清晰

- https://github.com/ratatui/ratatui : 终端下的图形化框架

## rust 的工具使用
<!-- 6c30a518-ba81-4b64-8200-4a6a3d223208 -->

cargo clippy 和 rust analyzer 是什么关系?

- rust-analyzer 没有复用 rustc ， 它是独立实现的分析器（为了速度）
- clippy 是单独的分析工具:
	- "rust-analyzer.check.command": "clippy"

## dyn 关键字的作用是什么?
<!-- e06b3732-f929-4fa0-ac25-fa4d47fff900 -->

dyn 的底层实现：胖指针（fat pointer）

&dyn Trait 并不是普通指针，而是 两个指针：
(data_ptr, vtable_ptr)

- data_ptr：指向真实对象
- vtable_ptr：指向该类型对应的 vtable ，包括:
	- 方法函数指针
	- drop 函数
	- 对齐信息、大小信息

通过 dyn Trait 来实现动态分发

## rust : 如何创建一个带有 string member 的结构体
<!-- ce05465f-069c-41f0-af23-a3fc6615ba0f -->

哦，看这个例子就发现比想象的复杂了:

[How to create a Rust struct with string members?](https://stackoverflow.com/questions/25754863/how-to-create-a-rust-struct-with-string-members)
```rust
struct Foo {
    bar: String,
    baz: &'static str,
}

fn main() {
    let foo = Foo {
        bar: "bar".to_string(),
        baz: "baz",
    };
    println!("{}, {}", foo.bar, foo.baz);
}
```

作为对比，更加通用的写法:
```rust
struct Foo<'a> {
    bar: String,
    baz: &'a str,
}

let s = String::from("hello");

let f = Foo {
    bar: "x".to_string(),
    baz: &s,
};
```


## rust 中这些指针有什么区别?
<!-- 33b972b6-b267-4f44-a28a-74af9d2c44cd -->

### 1. `&T`（共享引用 / immutable reference）
- **语义**：不可变借用（read-only view）。
- **所有权**：不拥有数据，仅临时借用；原 owner 依然有效。
- **可变性**：不可变（不能修改所指数据）。
- **线程安全**：`&T` 本身是 `Sync + Send`（若 `T` 满足）。
- **生命周期**：必须有明确的生命周期（编译时检查）。
- **零成本抽象**：编译后等价于裸指针，无运行时开销。
- **示例**：
  ```rust
  let x = 5;
  let r: &i32 = &x; // 只读引用
  ```

### 2. `&mut T`（可变引用 / mutable reference）
- **语义**：可变借用（独占写权限）。
- **所有权**：不拥有数据，但要求 **唯一访问权**（不能同时存在其他 `&` 或 `&mut`）。
- **可变性**：可修改数据。
- **线程安全**：`&mut T` 是 `Send` 但 **不是 `Sync`**（因为可变，不能跨线程共享）。
- **生命周期**：同样受生命周期约束。
- **示例**：
  ```rust
  let mut x = 5;
  let r: &mut i32 = &mut x;
  *r += 1;
  ```

Rust 的 **“别名 XOR 可变”** 原则：要么多个不可变引用，要么一个可变引用。

### 3. `Box<T>`（堆分配的独占所有权指针）
- **语义**：拥有堆上数据的 **唯一所有权**。
- **内存位置**：数据在堆上。
- **所有权**：独占（move 语义）；`Box` 被 drop 时自动释放堆内存。
- **可变性**：可通过 `*box = ...` 或 `box.field = ...` 修改。
- **线程安全**：`Box<T>` 是 `Send`（若 `T: Send`），但 **不是 `Sync`**（因为可变）。
- **用途**：
  - 递归数据结构（如链表、树）
  - 将大对象移到堆上
  - trait object（`Box<dyn Trait>`）
- **示例**：
  ```rust
  let b = Box::new(42);
  ```

### 4. `Rc<T>`（引用计数智能指针，单线程）
- **语义**：**单线程**下的 **共享所有权**（多个 owner）。
- **内存管理**：内部引用计数，计数归零时 drop。
- **可变性**：**不可变**（`Rc<T>` 本身不提供内部可变性；需配合 `RefCell`）。
- **线程安全**：**不是 `Send` 也不是 `Sync`**（非原子操作，线程不安全）。
- **用途**：单线程内多个地方共享只读（或内部可变）数据。
- **示例**：
  ```rust
  use std::rc::Rc;
  let a = Rc::new(42);
  let b = Rc::clone(&a); // 增加引用计数
  ```

想要内部可变？用 `Rc<RefCell<T>>`。

### 5. `Arc<T>`（原子引用计数，多线程版 `Rc`）
- **语义**：**多线程安全**的共享所有权。
- **内存管理**：原子操作的引用计数（`Arc` = **Atomically Reference Counted**）。
- **可变性**：同样不可变，需配合 `Mutex<T>` 或 `RwLock<T>` 实现内部可变。
- **线程安全**：`Arc<T>` 是 `Send + Sync`（若 `T: Send + Sync`）。
- **开销**：原子操作比 `Rc` 稍慢。
- **用途**：跨线程共享数据。
- **示例**：
  ```rust
  use std::sync::Arc;
  use std::thread;
  let data = Arc::new(42);
  let d1 = Arc::clone(&data);
  thread::spawn(move || println!("{}", d1));
  ```

> ✅ 多线程共享可变状态？用 `Arc<Mutex<T>>` 或 `Arc<RwLock<T>>`。

### 6. `*const T` 和 `*mut T`（原始指针）
- **语义**：C 风格的裸指针（raw pointers）。
- **安全性**：**不安全**（`unsafe`），绕过 Rust 所有权和借用检查。
- **可空**：可以为 `null`。
- **生命周期**：无生命周期约束。
- **可变性**：
  - `*const T`：通常用于只读（但可通过 `unsafe` 转为可写）
  - `*mut T`：用于可写
- **线程安全**：无保证，完全由程序员负责。
- **用途**：
  - FFI（调用 C 库）
  - 实现 unsafe 数据结构（如自定义分配器）
  - 与硬件/内核交互（如你熟悉的 Linux 内核开发）
- **示例**：
  ```rust
  let x = 5;
  let p: *const i32 = &x as *const _;
  unsafe { println!("{}", *p); }
  ```

### 7. `NonNull<T>`（非空原始指针的封装）
- **语义**：`*mut T` 的安全封装，**保证非空（non-null）**。
- **优势**：
  - 与 `Option<NonNull<T>>` 可做 ** niche optimization **（`None` 用 null 表示，不占用额外空间）
  - 比裸指针更安全（编译器知道它非空）
- **用途**：
  - 实现高性能数据结构（如链表、自定义分配器）
  - 标准库内部广泛使用（如 `Box`, `Vec` 的底层）
- **注意**：仍属于 `unsafe` 范畴，但比裸指针更“友好”。
- **示例**：
  ```rust
  use std::ptr::NonNull;
  let x = 42;
  let ptr = NonNull::from(&x); // 安全地从引用创建
  ```

### 对比总结表

| 类型         | 所有权       | 可变性       | 线程安全          | 内存位置 | 安全性   | 典型用途                   |
|--------------|--------------|--------------|-------------------|----------|----------|----------------------------|
| `&T`         | 借用（共享） | 不可变       | 是（若 `T` 满足） | 任意     | 安全     | 函数参数、遍历             |
| `&mut T`     | 借用（独占） | 可变         | 否（非 `Sync`）   | 任意     | 安全     | 修改局部数据               |
| `Box<T>`     | 独占         | 可变         | `Send`（若 `T`）  | 堆       | 安全     | 递归结构、trait object     |
| `Rc<T>`      | 共享         | 不可变*      | 否                | 堆       | 安全     | 单线程共享（+ `RefCell`）  |
| `Arc<T>`     | 共享         | 不可变*      | 是（若 `T`）      | 堆       | 安全     | 多线程共享（+ `Mutex`）    |
| `*const T`   | 无           | 只读（名义） | 无保证            | 任意     | `unsafe` | FFI、内核编程              |
| `*mut T`     | 无           | 可变         | 无保证            | 任意     | `unsafe` | FFI、系统编程              |
| `NonNull<T>` | 无           | 可变         | 无保证            | 任意     | `unsafe` | 高性能数据结构、标准库实现 |

> *注：`Rc`/`Arc` 本身不可变，但可包装 `RefCell`/`Mutex` 实现内部可变性。

选择建议（结合你的背景）：
- **系统编程/内核模块**：常用 `&T`, `&mut T`, `Box`, 以及 `*mut T`（配合 `unsafe`）。
- **高性能共享结构**：单线程用 `Rc<RefCell<T>>`，多线程用 `Arc<Mutex<T>>`。
- **避免不必要的堆分配**：优先用引用而非 `Box`/`Rc`。
- **FFI 或底层操作**：用 `*mut T` 或 `NonNull<T>`，但务必确保内存安全。


## RefCell 的作用是什么
<!-- 3d9290b3-b40b-43d1-a7d2-c0a7393389fd -->

RefCell 把 Rust 的“借用检查”从编译期推迟到了运行期。

最好是在编译期强制检查借用规则。但在一些场景中：
- 数据结构是 自引用 / 图结构
- 接口是 &self，但内部需要修改
- 所有权关系在运行期才能确定

编译器 无法静态证明安全性，于是你会被拒绝：

```rust
struct Node {
    next: Option<Box<Node>>,
}

impl Node {
    fn set_next(&self, n: Node) {
        self.next = Some(Box::new(n)); // ❌ 编译失败
    }
}
```

你“逻辑上”知道是安全的，但编译器不知道。

这时就需要 内部可变性（Interior Mutability）。

`RefCell<T>` 是什么（本质）
```rust
use std::cell::RefCell;
let x = RefCell::new(5);
```
`RefCell<T>` 提供的是：
- 通过 &self 在运行时动态地检查借用规则而不是在编译期。


RefCell 的实现原理是:
```rust
struct RefCell<T> {
    value: T,
    borrow_flag: isize, // 借用计数
}
```
- borrow_flag > 0：存在 N 个 &T
- borrow_flag == 0：没有借用
- borrow_flag == -1：存在一个 &mut T


典型使用方法:

1. &self 中修改内部状态
```rust
struct Cache {
    map: RefCell<HashMap<String, i32>>,
}

impl Cache {
    fn insert(&self, k: String, v: i32) {
        self.map.borrow_mut().insert(k, v);
    }
}
```

2. 与 Rc 配合（单线程共享可变）
```rust
use std::rc::Rc;
use std::cell::RefCell;

let data = Rc::new(RefCell::new(0));

let a = Rc::clone(&data);
let b = Rc::clone(&data);

*a.borrow_mut() += 1;
*b.borrow_mut() += 1;
```

- Rc：解决 多所有者
- RefCell：解决 可变性

## Cell 和 RefCell
<!-- 142a919b-66ca-4afb-ae30-bfa8d54ad85f -->

`Cell` 和 `RefCell` 是 Rust 中用于实现 **内部可变性（Interior Mutability）** 的两种类型，即在 **不可变引用（`&T`）** 的情况下也能修改内部数据。
这与 Rust 默认的借用规则（不可变引用不能修改数据）相悖，因此它们通过运行时检查（`RefCell`）或限制（`Cell` 只能用于 `Copy` 类型）来保证内存安全。

下面分别解释并附示例。

### 1. `Cell<T>`
- 只适用于实现了 `Copy` trait 的类型（如 `i32`, `bool`, `f64` 等）。
	- 注意：`Cell` 无法用于非 `Copy` 类型（如 `String`、`Vec`），因为 `get()` 需要复制数据。
- 不可获取内部值的引用（即没有 `get_ref()`），只能通过 `get()` 拷贝值、`set()` 覆盖值。
- **无运行时开销**（零成本抽象）。

### 2. `RefCell<T>`
- 适用于任意类型（包括非 `Copy`）。
- 允许通过 `borrow()` 获取不可变引用，`borrow_mut()` 获取可变引用。
- **在运行时检查借用规则**：如果违反“同时只能有一个可变引用或多个不可变引用”的规则，会 panic。
- 有轻微运行时开销（计数器检查）。

### 何时使用？
- 需要**在不可变上下文中修改数据**（比如在 `Rc<T>` 共享所有权时）。
- `Cell`：用于 `Copy` 类型，性能优先。
- `RefCell`：通用，支持任意类型，但运行时检查。

注意：`RefCell` 不是线程安全的。多线程应使用 `Mutex` 或 `RwLock`。

总结：
- `Cell`: 适合 `Copy` 类型，无运行时开销，只能 `get/set`。
- `RefCell`: 适合任意类型，运行时借用检查，使用 `borrow`/`borrow_mut`。

两者都打破了“不可变引用不可修改”的限制，但通过不同机制保证安全。

## rust options
<!-- 3b656004-e667-45cf-b1a5-02dddcc2ebd8 -->

![](https://i.redd.it/dt1wan02aa561.png)

一共四个:
- convert : option<&T>
- transform : 另外的类型
- export : 不太理解了
- access : 原始的

## IntoIter IterMut 和 Iter 的区别是什么?
<!-- e7358304-27b4-488f-81ec-03c096fe7f1e -->

Also, there's actually 3 different kinds of iterator each collection should endeavour to implement:

| 迭代器     | 取得什么 | 是否消耗容器 | 是否可修改元素   |
| ---------- | -------- | ------       | --------         |
| `Iter`     | `&T`     | 否           | 否               |
| `IterMut`  | `&mut T` | 否           | 是               |
| `IntoIter` | `T`      | 是           | 是（通过所有权） |

```rust
let v = vec![1, 2, 3];

for x in v.iter() {
    println!("{}", x);
}

println!("{:?}", v); // v 仍然可用
```

```rust
let mut v = vec![1, 2, 3];

for x in v.iter_mut() {
    *x *= 10;
}

println!("{:?}", v); // [10, 20, 30]
```

```rust
let v = vec![1, 2, 3];

for x in v.into_iter() {
    println!("{}", x);
}

// println!("{:?}", v); // ❌ v 已被 move
```

那么 for 循环到底是用的哪一个:
```rust
for x in v { }        // v: Vec<T> → IntoIter<T>
for x in &v { }       // &Vec<T> → Iter<T>
for x in &mut v { }   // &mut Vec<T> → IterMut<T>
```

## 生命周期 和 所有权
1. Rust 永远也不会自动创建数据的 “深拷贝”， 同时使第一个变量无效了
2. 参数和返回值都会 转移所有权
3. 变量的所有权总是遵循相同的模式：将值赋给另一个变量时移动它。当持有堆中数据值的变量离开作用域时，其值将通过 drop 被清理掉，除非数据被移动为另一个变量所有
4. 可以同时出现多个不可变引用 或者 一个可变ref，　可变引用有一个很大的限制：在特定作用域中的特定数据有且只有一个可变引用
5. 我们 也 不能在拥有不可变引用的同时拥有可变引用。不可变引用的用户可不希望在他们的眼皮底下值就被意外的改变了！然而，多个不可变引用是可以的，因为没有哪个只能读取数据的人有能力影响其他人读取到的数据。
7. 生命周期确保结构体引用的数据有效性跟结构体本身保持一致。
8. 在给出接收者和方法名的前提下，Rust 可以明确地计算出方法是仅仅读取（&self），做出修改（&mut self）或者是获取所有权（self）。事实上，Rust 对方法接收者的隐式借用让所有权在实践中更友好。
9. 生命周期使用的位置:
    1. 函数: 返回引用指定生命周期
    2. 结构体: 结构体成员包含的引用
    3. trait

10. 泛型使用的位置:
    1. 函数: 泛型函数
    2. 结构体
    3. trait
11. 泛型和生命周期的描述符都是可以同时使用的, 泛型可以添加限制，限制内容包括生命周期和trait, 但是不可以是结构体

12. 引用到底是什么东西:
  1. 是实现 borrow, 而不是取地址的作用
  2. deref
  3. 解引用强制多态


14. leetcode 688 中 对于从queue 中间去处元素的操作，
```
  let x = match queue.get(i) {
      Some(x)=> *x, // 这样的话，是进行了 clone 吗 ?
      None => panic!("Access queue out of order, impossible !")
  };
```

15. 如何在循环中间获取 mutable borrow
    - https://stackoverflow.com/questions/44104519/rust-lifetime-issue-in-loop 这一个例子看不懂啊!
    - https://stackoverflow.com/questions/37986640/cannot-obtain-a-mutable-reference-when-iterating-a-recursive-structure-cannot-b 这一个例子似乎是曾经含有错误，但是现在已经可以正常编译了, 是由于 non lexical lifetime

16. non-lexical-lifetimes
    - https://doc.rust-lang.org/edition-guide/rust-2018/ownership-and-lifetimes/non-lexical-lifetimes.html 官方文档
    - https://stackoverflow.com/questions/50111949/mutable-borrow-in-a-loop


17. leetcode 688 中能否使用更加优雅的指针的方法而不是采用 x y 重复的代码

18. https://rust-unofficial.github.io/too-many-lists/first-ownership.html 说明 mutable reference 对于读也是 exclusive 的，而且 borrow 的含义说明，当使用完成之后，限制就会解除

19. When you invoke `self.item.increment(amount)`, you are trying to pass `self.item` by value, thus moving ownership to the `Item::increment` function, but you are not allowed to do this with references to values that you don't own.
    1. 看上去，move 和 mut borrow 看似都是让某一个用户独占该变量，但是关键的不同在于，borrow 会归还，但是move 不会归还了。
    2. 如果将所有权传递给函数，那么函数调用结束之后，该变量就被释放了

20. By default, **a pattern match** will try to move its contents into the new branch, but we can't do this because we don't own self by-value here.

21. https://stackoverflow.com/questions/27197938/memreplace-in-rust mem::replace 解决了什么问题，为什么其本身需要通过unsafe机制实现
    1. 如果想要实现赋值操作，要么实现clone，要么move ownership
    2. 没有办法保证移动 ownership 之后立刻销毁了，那么之后使用变量的人就GG
    3. replace 操作保证转移了 ownership 之后总是含有发新的赋值，只是变量发生了变化


23. It turns out that **writing the argument of the closure that way doesn't specify that value is a mutable reference**.
Instead, it creates a pattern that will be matched against the argument to the closure; |&mut value| means "the argument is a mutable reference,
but just copy the value it points to into value, please." If we just use |value|, the type of value will be &mut i32 and we can actually mutate the head.
        list.peek_mut().map(|value| *value = 42); // 正确
        list.peek_mut().map(|mut value| *value = 42); // @todo 诡异的警告
        list.peek_mut().map(|&mut value| *value = 42); // 报错，和上述的理论相符

24. Quite simply, a lifetime is the name of a region (~block/scope) of code somewhere in a program. That's it. When a reference is tagged with a lifetime, we're saying that it has to be valid for that entire region. Different things place requirements on how long a reference must and can be valid for. The entire lifetime system is in turn just a constraint-solving system that tries to minimize the region of every reference.
    1. 生命周期是为了处理ref 而产生的

25. Within a function body you generally can't talk about lifetimes, and wouldn't want to anyway. The compiler has full information and can infer all the contraints to find the minimum lifetimes. However at the type and API-level, the compiler doesn't have all the information. It requires you to tell it about the relationship between different lifetimes so it can figure out what you're doing.
    1. 函数中间的确没有，更多是函数生命和结构体申明
    2. 函数需要生命周期的典型 : 传递两个不同生命周期的ref ，返回其中的一个ref
    3. 结构体需要生命周期的典型 : 当结构体中间持有ref, 需要指明结构体和其中ref 生命周期的关系. 思考一下:C++中间是如何处理class中间的ref 的

26. So if you keep the output around for a long time, this will expand the region that the input must be valid for.
Once you stop using the output, the compiler will know it's ok for the input to become invalid too.
    1. 生命周期限定符辅助输入的生命周期的, 因为output lifetime 依赖于 input lifetime ，只要 output 没有被 disable ，那么 input 禁止被disable
    2. 有一个错觉，那就是引用会到scope/block 结束的位置的时候消失，但是实际上并不是的，消失的位置是在无人使用的时候

27. https://users.rust-lang.org/t/what-is-the-point-of-lifetime-parameters-in-struct-impl-blocks/14631/2
    1. struct 的生命周期
    2. subtype 的含义是什么




## 字符串
1. “字符串 slice” 的类型声明写作 &str
2. String 和 str http://www.ameyalokare.com/rust/2017/10/12/rust-str-vs-String.html
3. Prefer &str as a function parameter or if you want a read-only view of a string; String when you want to own and mutate a string.
4. String 类型的slice 就是 str, 因为str本身描述不可变的，所以slice 作为没有所有权的类型是很科学的
5. 可以使用 to_string 方法，它能用于任何实现了 Display trait 的类型，字符串字面值也实现了它. 也可以使用 String::from 函数来从字符串字面值创建 String
6. 也可以使用 String::from 函数来从字符串字面值创建 String
7. 使用+运算符或!format宏拼接字符串

## malloc
除了数据被储存在堆上而不是栈上之外，box 没有性能损失。不过也没有很多额外的功能。它们多用于如下场景
1. 当有一个在编译时未知大小的类型，而又想要在需要确切大小的上下文中使用这个类型值的时候 : 递归类型：因为Box 将真正的数据存储在heap 上，自己只是一个指针，所以可以结束无限循环
2. 当有大量数据并希望在确保数据不被拷贝的情况下转移所有权的时候 :
3. 当希望拥有一个值并只关心它的类型是否实现了特定 trait 而不是其具体类型的时候 : 17 trait 对象

4. Box的作用，malloc 的效果的效果，可以使用ref 的形式的访问，只是将真正的数据放到heap之中，borrow ref的规则依旧满足.
5. Rc的作用，可以实现多个人访问，使用clone 实现多个访问, 和Box相同的，访问直接使用`*` 就可以了，但是似乎仅仅限于
6. Rc provides shared ownership so by default its contents can't be mutated, while Box provides exclusive ownership and thus mutation is allowed:

```rs
use std::rc::Rc;

#[derive(Debug)]
struct A{
    name: i32,
    addr: i32,
}

fn main() {
    let mut b = A{name : 2, addr : 3};
    let mut x = Rc::new(&mut b);
    let mut c = Rc::get_mut(&mut x).unwrap();
    c.name = 4;
    // b.name = 5; 取消注释之后失败，因为x 的初始化的时候已经获取了其mut 的，所以无法在修改，如果是获取其ownership，将print 报错
    println!("{:#?}", c);
    println!("{:#?}", b);
}
// x的初始化参数必须是 &mut ,　没有mut , c.name 的赋值失败，没有&, 最后一行无法输出



use std::rc::Rc;

fn main() {
    let b = 12;
    let mut x = Rc::new( b);
    let c = Rc::get_mut(&mut x).unwrap();
    *c = 4;
    println!("{}", c);
    println!("{}", b);
}
```
7. is a & reference so the data it refers to cannot be written : WDNMD, 出现该错误的原因并不是 &不可以访问，需要进一步deref (感觉这一个rust 的deref 本身有点问题)，真正的原因是需要成为mutable 的。
成为mutable 的要求是一个chain.

```rs
fn main() {
   let mut i = Box::new(12); // 必须含有 mut
   *i = 12;
}
```

```rs
fn main() {
   let mut b = 12;
   let mut i = Box::new(b);
   *i = 13;
   println!("{}", b);
   b = 14;
   println!("{}", b);
}
// 1. 没有报错
// 2. 输出 12 14, 不是13 14 说明 i 和 b　已经分离
真正的原因是: Box::new的操作会调用Copy(应该是，没有证据)，将数据copy 到Heap 上面，而原来的数据还是在stack 上，
将b的申明中间的mut 删除掉之后，只是b 不能继续赋值了。
```
8. Rc RefCell 的clone 参数必定是ref(因为需要实现多个ownership, 而且clone 的参数要求)，但是包括Box在内的new 的参数 不能是ref
9. Box::new Rc::new 的对象，前者可以直接使用 `*`, 后者需要 get_mut 才可以。其中的原因是，Box 的ownership 是独占的，但是 Rc 必须保证只有一个 strong_accout的时候才可以进行修改. 如果想要实现多个人修改，那么就需要采用ref cell

```rs
#[derive(Debug)]
struct A{
    name: i32,
    addr: i32,
}

fn main() {
    let mut b = A{name : 2, addr : 3};
    let mut x = Box::new(&mut b);
    x.name = 4; // 再一次说明 Box 可以直接访问发，而且更加过分的是，没有使用(*x).name
    println!("{:#?}", x);
    println!("{:#?}", b);
}
```

```rs
#[derive(Debug)]
struct A{
    name: i32,
    addr: i32,
}

fn main() {
    let b = A{name : 0, addr : 0};
    let mut x = Box::new(b);
    x.name = 4;
    println!("{:#?}", x);
    // println!("{:#?}", b);
}
// 写法有点奇怪了，一般来说需要将b的声明添加mut 就可以 实现对于b的数值进行修改了，其实并没有造成误解，因为
// b 的ownership 已经没有，而 x 必须含有mut
```

```rs
fn main() {
    let mut b = A{name : 0, addr : 0};
    let mut x = Box::new(&b);
    // x.name = 4; // 此行出现错误，报错会说，这是ref, 其实是mutable,传入Box::new不是mutable 的，所以自然不可以改变。
    println!("{:#?}", x); // 但是读取操作并不影响，依旧是可以使用的
    // println!("{:#?}", b);
}
```

10. borrow 之后就不可以再转移ownership了，没有ownership 变量相当于消失了，没有mut 的之后，还可以打印一下。

11. Rc<RefCell> 还是 RefCell<Rc> ： 可变多个

## rust trait
<!-- 71a576a5-22d5-4ccb-913e-fe061a93f7b2 -->

如果一个 trait 中所有的方法有如下属性时，则该 *trait 是对象安全*的：
    1. 返回值类型不为 Self
    2. 方法没有任何泛型类型参数

并非所有 trait 都能变成 dyn Trait：
对象安全（Object Safety）规则：
- 方法不能返回 Self
- 方法不能使用泛型参数
- Self: Sized 方法不能用于 trait object

```rust
trait Foo {
    fn ok(&self);
    fn bad<T>(&self, t: T); // ❌ 破坏对象安全
}
```

trait 的高级构建:
1. 继承（Supertrait）
```rust
trait Write: Read {
    fn write(&mut self, buf: &[u8]);
}
```
Write 的实现者必须先实现 Read

2. Blanket impl（泛化实现）
```rust
impl<T: Display> ToString for T {
    fn to_string(&self) -> String {
        format!("{}", self)
    }
}
```

3. Marker trait（零方法 trait）
```rust
trait Send {}
trait Sync {}
```
作用：
- 参与类型系统判断
- 不提供行为，仅提供“属性”

## as_ref
<!-- 7360269a-090c-4a42-ac7b-a3552bbf9a2e -->

as_ref 的作用是什么: as_ref 和 map 的文档可以看一下，很恐怖的
```
    let text: Option<String> = Some("Hello, world!".to_string());
    // First, cast `Option<String>` to `Option<&String>` with `as_ref`,
    // then consume *that* with `map`, leaving `text` on the stack.
    let text_length: Option<usize> = text.as_ref().map(|s| s.len());
    println!("still can print text: {:?}", text);
```
> 其中 as_ref 和 take() 的功能类似

(我靠，太 nb 了，看看神奇海螺的意见)

在 Rust 中，`as_ref` 的本质是：**在不发生所有权转移的前提下，
把一个“拥有者或容器”借用为其内部值的引用**。它并不是一个关键字，
而是 **`AsRef` trait 的统一接口**。

要准确理解它，需要从 **语义、类型系统、常见实现模式、与解引用的区别** 四个层面来看。


### 一、定义层面：`as_ref` 是什么

`as_ref` 来自标准库 trait：

```rust
pub trait AsRef<T: ?Sized> {
    fn as_ref(&self) -> &T;
}
```

核心语义只有一句话：

> **给我一个 `&Self`，我给你一个 `&T`**

关键点：

* **不转移所有权**
* **只做借用**
* **通常是“视图转换（view conversion）”**

### 二、直观理解：把“外层”借成“内层”

#### 1. 最常见例子：`Option<T>`

```rust
let opt: Option<String> = Some("hello".to_string());

let r1: Option<&String> = opt.as_ref();
```

这里发生了什么？

```text
Option<String>   --as_ref-->   Option<&String>
```

含义是：

* `opt` 仍然拥有 `String`
* 你只是借用了里面的 `String`

如果不用 `as_ref`：

```rust
let r2 = opt.map(|s| s); // ❌ 移动 String
```

---

#### 2. `Result<T, E>`

```rust
let r: Result<String, i32> = Ok("ok".to_string());

let r_ref: Result<&String, &i32> = r.as_ref();
```

同样规则：

* `Ok(T)` → `Ok(&T)`
* `Err(E)` → `Err(&E)`

---

### 三、`AsRef` 的设计目的

#### 1. 用于“参数泛化”，而不是逻辑转换

`AsRef` 的主要使用场景是 **函数参数抽象**：

```rust
fn open<P: AsRef<Path>>(path: P) {
    let p: &Path = path.as_ref();
}
```

这样调用者可以传：

```rust
open("foo.txt");        // &str
open(String::from("a"));// String
open(PathBuf::from("b"));
```

**设计思想：**

* 函数内部只关心 `&Path`
* 调用者自由决定传什么

---

#### 2. 为什么不是 `Into<&T>`？

因为 `Into` / `From` **通常涉及所有权语义**，而：

> `AsRef` 明确保证：只返回引用，不发生移动

---

### 四、`as_ref` vs 解引用（`&*`）

这是一个关键区别点。

#### 1. 解引用是“语法层的指针操作”

```rust
let s: String = "hi".to_string();
let r: &str = &*s;
```

这是：

* 先 `Deref<Target = str>`
* 再取引用

#### 2. `as_ref` 是“trait 层的语义转换”

```rust
let r: &str = s.as_ref();
```

差异总结：

| 维度     | `as_ref` | `&*x`      |
| ------ | -------- | ---------- |
| 抽象层级   | trait    | 语法 / Deref |
| 可泛化    | 是        | 否          |
| 适合 API | 是        | 否          |
| 可读性    | 高        | 低          |

> **公共 API 中优先使用 `AsRef`，而不是依赖解引用细节。**

---

### 五、`as_ref` 在泛型代码中的真实价值

#### 1. 典型模式：避免 clone / move

```rust
fn process<S: AsRef<str>>(s: S) {
    let s: &str = s.as_ref();
    println!("{}", s);
}
```

如果不用 `AsRef`，你通常会写：

```rust
fn process(s: &str) { }
```

这会导致调用端必须显式借用：

```rust
process(&string);
```

而 `AsRef` 让 API 更“宽松”。

---

#### 2. Option / Result 中的链式调用

```rust
let name: Option<String> = get_name();

let len = name
    .as_ref()
    .map(|s| s.len());
```

这里 `as_ref` 的作用是：

> **把“消费 Option”变成“借用 Option”**

---

### 六、`as_ref` vs `borrow`（Borrow trait）

两者容易混淆，但目标不同：

| trait       | 目的                  |
| ----------- | ------------------- |
| `AsRef<T>`  | 轻量、视图转换             |
| `Borrow<T>` | 等价性保证（Hash / Eq 一致） |

`Borrow` 常用于：

```rust
HashMap<String, V>
map.get("key"); // &str
```

这里要求：

* `String` 和 `&str` 的 `Hash` / `Eq` 行为一致

而 `AsRef` **不保证等价性**。

### 七、什么时候不该用 `as_ref`

1. **你需要所有权**

   * 用 `into()` / `to_owned()`

2. **你依赖具体类型行为**

   * 直接用具体类型

3. **trait 语义不清晰**

   * `AsRef<Foo>` 但 `Foo` 不明显是“视图目标”

### 八、一句话总结

> **`as_ref` 是 Rust 中“把拥有值或容器，安全地借用为其内部值引用”的标准接口，用于消除不必要的所有权、提升 API 泛化能力。**

如果你愿意，下一步我可以：

* 对比 `as_ref / as_mut / into / to_owned`
* 用 MIR 角度解释 `as_ref` 是否零成本
* 总结“什么时候 API 参数该用 `AsRef`”的设计准则

## 模式匹配
1. 注意, 只使用 `_` 和使用以下划线开头的名称有些微妙的不同：比如 `_x` 仍会将值绑定到变量，而 `_` 则完全不会绑定。

## rust 中 `static` and `const` 的区别
<!-- 7666cd59-dda5-4342-a55d-5cc047be3054 -->

https://stackoverflow.com/questions/52751597/what-is-the-difference-between-a-constant-and-a-static-variable-and-which-should

正如 RFC 中说明的那样 : https://github.com/rust-lang/rfcs/blob/master/text/0246-const-vs-static.md

> `constants` declare constant values. These represent a value, not a memory address. This is the most common thing one would reach for and would replace static as we know it today in almost all cases.
> `statics` declare global variables. These represent a memory address. They would be rarely used: the primary use cases are global locks, global atomic counters, and interfacing with legacy C libraries.

## rust 的 Vector 如何赋值

### Question
```rust
  let arr: [i32; 5] = [1, 2, 3, 4, 5];
  let mut guess = String::new();
  io::stdin().read_line(&mut guess)
      .expect("Failed to read line");
  let guess: i32 = guess.trim().parse().expect("fuck learn this is not easy");
  println!("fuck this line {} {}", arr[guess], guess);
```

### Return
what is the purpose of keyword `return`
how to return in match
```rust
    fn get_number(c: char) -> i32 {
        match c {
            'I' => return 2,
            _ => return 3,
        }
        2
    }
```

```rust
use std::collections::BTreeMap;

// lost control of reference
pub fn majority_element(nums: Vec<i32>) -> i32 {
    // why should use divide and conquer
    let threshold = nums.len() as i32 / 2;
    let mut scores = BTreeMap::new();
    for i in &nums {
        match scores.get(i).cloned() {
            Some(x) => scores.insert(*i, x + 1),
            None => scores.insert(*i, 1),
        };
    }

    for (key, value) in &scores {
        // println!("{}: {}", key, value);
        if *value > threshold {
            return *key;
            // this line can not be changed to ---> *key
        }
    }
    1 as i32
}

fn main() {
    let v = vec![2, 3, 2];
    println!("{}", majority_element(v));
}
```

#### clone() how to copy and clone a data
https://stackoverflow.com/questions/28800121/use-of-moved-values
When a function requires a parameter by value, the compiler will check if the value can be copied by checking if it implements the trait Copy.
1. If it does, the value is copied (with a memcpy) and given to the function, and you can still continue to use your original value.
2. If it doesn't, then the value is moved to the given function, and the caller cannot use it afterwards


## Rust for leetcode
1. 创建指定大小的初始化数组 vec![0; 26]
2. iter String
```
        let bytes = word.as_bytes();
        let mut v = &mut self.node;

        for (_, &item) in bytes.iter().enumerate() {
```

## rust copy trait
<!-- 232dcdb3-0239-4043-8b3d-ccc188eaa8a4 -->

添加一个例子测试下吧

## unwrap
<!-- 83aba80a-eb79-4942-a020-11d4ee14ec57 -->

https://blog.cloudflare.com/zh-cn/18-november-2025-outage/

## closure
<!-- b059b6dc-ec62-4fe0-b39b-f15c51ff9492 -->

关联测试 test_closure

还是感觉不得要领

## prelude
<!-- f437f9b8-41a5-49ea-863b-2cc3f09038b3 -->
https://rustwiki.org/zh-CN/std/prelude/index.html

## coercions
<!-- 29faa2ca-8e02-48d7-8f51-e17306d5d4a1 -->

https://www.possiblerust.com/guide/what-can-coerce-and-where-in-rust

## ownership 的理解
<!-- 70a9d47b-52eb-457c-b0bc-024be92a659d -->

```txt
写法                   含义         调用后原变量
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 fn method(self)        获取所有权   ❌ 不能再使用（被消耗）
 fn method(&self)       不可变借用   ✅ 可以继续使用
 fn method(&mut self)   可变借用     ✅ 可以继续使用
```

也就是说，如果定义了
```rust
    pub fn request_review(self) -> PendingReviewPost {
        PendingReviewPost {
            content: self.content,
        }
    }
```
然后这样调用:
```rust
let draft = DraftPost::new();
let pending = draft.request_review();  // draft 被消耗

// 编译错误！防止你在草稿状态已结束后还操作它
draft.add_text("more");  // ❌ value borrowed here after move
```

## 难怪没搞懂，原来是由于 rust book 后面的章节都没看啊

## ./AGENTS.md 来自于 https://github.com/boxlite-ai/boxlite
写的真好啊

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
