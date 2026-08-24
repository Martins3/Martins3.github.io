# Rust 中 Move、Copy、Clone 和 Drop 的关系
<!-- 959fbd32-4f5a-415f-ba0d-6da8a8d1aeb8 -->

看 stackoverflow 的这个回答说明核心区别:
- https://stackoverflow.com/questions/31012923/what-is-the-difference-between-copy-and-clone

简单来说 Clone 和 Copy 本质就是深度拷贝和潜拷贝
Copy 就是简单的 memcpy

可以把 `Copy`、`Clone`、`Drop` 放到 Rust 的所有权体系里理解。它们分别回答三个问题：

| 概念    | 回答的问题           | 是否显式调用    | 是否允许自定义行为 |
| ------- | -------------------- | --------------- | ------------------ |
| Move    | 所有权转交给谁       | 隐式            | 否                 |
| `Copy`  | 移动时能否保留原值   | 隐式            | 否                 |
| `Clone` | 能否主动创建一个新值 | 显式 `.clone()` | 是                 |
| `Drop`  | 值失效时如何清理     | 隐式            | 是                 |

可以解释
1. Copy 不可以自定义，因为 memcpy 没有什么操作空间
2. Copy 和 Drop 是冲突的
3. Clone 和 Drop 可以同时存在

因为 每次 `clone()` 都必须产生一个能够独立完成其销毁责任的新值，
要么深度拷贝，要么增加引用计数，所以新的对象就是让 Drop 来操作的

4. Copy 和 Drop 为什么冲突
	- Copy 制作了两个对象，但是 Drop 两次，很有可能是 double free


## = 的含义

右侧类型实现 Copy     => 复制，左右两边都能继续使用
右侧类型没有实现 Copy => move，右侧原位置失效

= 本身不会调用 Clone。

## Move 是默认行为

Move 不是 trait，而是 Rust 默认的所有权转移方式。

```rust
let a = String::from("hello");
let b = a;

// println!("{a}"); // 错误：a 已经被移动
println!("{b}");
```

这里没有复制字符串，只是把字符串的所有权从 `a` 转交给了
`b`。作用域结束时，只会销毁 `b`：

```text
a --move--> b --drop-->
```

这保证堆内存只释放一次。

即使一个类型没有实现 `Drop`，它的字段仍然可能需要销毁：

```rust
struct User {
    name: String,
}
```

`User` 没有手动实现 `Drop`，但编译器会生成 drop glue，在 `User` 销毁时自动销毁
`name`。

因此要区分：

- 实现了 `Drop`：类型有自定义析构函数。
- 需要 drop：类型本身或其字段需要执行清理。


## 典型类型对照

| 类型                 | `Copy` | `Clone`          | 需要清理 | 说明                                   |
| -------------------- | ------ | ---------------- | -------- | -------------------------------------- |
| `i32`                | 是     | 是               | 否       | 普通数值                               |
| `&T`                 | 是     | 是               | 否       | 复制共享引用                           |
| `&mut T`             | 否     | 否               | 否       | 避免产生多个可变引用                   |
| `String`             | 否     | 是               | 是       | Clone 深复制，离开作用域时释放内存     |
| `Vec<T>`             | 否     | 当 `T: Clone` 时 | 是       | 管理堆内存                             |
| `Box<T>`             | 否     | 当 `T: Clone` 时 | 是       | 独占堆对象                             |
| `Rc<T>`              | 否     | 是               | 是       | Clone 增计数，销毁时减计数             |
| `Arc<T>`             | 否     | 是               | 是       | 线程安全引用计数                       |
| `File`               | 否     | 否               | 是       | 销毁时关闭文件，复制需要 `try_clone()` |
| `MutexGuard`         | 否     | 否               | 是       | 销毁时解锁                             |
| 纯 `Copy` 字段结构体 | 可以   | 可以             | 否       | 适合派生 `Copy`                        |

`File`
是一个值得注意的例子。复制文件句柄有多种可能语义，成本和平台行为也不简单，所以它没有实现普通的
`Clone`，而是提供可能失败的 `try_clone()`。

## drop() 和 Drop::drop() 的区别

通常不能直接调用析构方法：

```rust,compile_fail
value.drop();
```

应该使用：

```rust,ignore
std::mem::drop(value);
```

`std::mem::drop()` 本身非常简单，本质是取得值的所有权，然后让它立即离开作用域：

```rust
pub fn drop<T>(_x: T) {}
```

例如提前释放锁：

```rust,ignore
let guard = mutex.lock().unwrap();

// 使用受保护的数据

drop(guard); // 提前解锁
```

而 `Drop::drop(&mut self)` 是编译器在值销毁过程中调用的析构方法。

## 析构顺序和移动

局部变量通常按照声明顺序的逆序销毁：

```rust
let a = String::from("a");
let b = String::from("b");

// 先销毁 b，再销毁 a
```

结构体字段则按照定义顺序销毁，而不是逆序。编译器还会使用 drop flags
记录值或字段是否已经被移动，避免再次销毁已经转移所有权的内容。

手动实现了 `Drop` 的类型不能随意移出某个字段：析构函数运行时通常假设整个 `self`
仍然完整。需要取出字段时，常用 `Option::take()`、`mem::replace()`
或更底层的手动析构方案。

## 与析构有关的其他概念

### std::mem::needs_drop::<T>()

用于判断类型是否可能需要析构：

```rust
use std::mem::needs_drop;

assert!(!needs_drop::<i32>());
assert!(needs_drop::<String>());
```

它主要适合底层容器和 unsafe 代码做优化，普通业务代码通常不需要使用。

### ManuallyDrop<T>

禁止编译器自动销毁内部值：

```rust
use std::mem::ManuallyDrop;

let value = ManuallyDrop::new(String::from("hello"));
```

这常用于 unsafe、联合体或手动管理字段销毁顺序。它不会让一个原本非 `Copy`
的资源安全地变成 `Copy`；开发者必须自己保证资源最终只释放一次。

### mem::forget()

消耗一个值，但不运行其析构函数：

```rust
std::mem::forget(String::from("hello"));
```

这会泄漏资源。它是安全函数，因为 Rust
的内存安全不能依赖析构一定执行，例如程序可能中止，引用计数也可能形成环。

### MaybeUninit<T>

表示一块可能尚未初始化为 `T` 的存储：

```rust
use std::mem::MaybeUninit;

let slot = MaybeUninit::<String>::uninit();
```

未初始化内容不会被当作 `T` 自动销毁。它主要用于底层初始化、FFI 和 unsafe
容器实现。

## 实用判断方法

设计类型时可以这样考虑：

- 只是数字、布尔值、坐标、标志位等纯数据：考虑 `Copy + Clone`。
- 拥有堆内存，但能够复制内容：实现 `Clone`，由字段自动清理或实现 `Drop`。
- 拥有独占资源，复制语义不明确：不实现 `Copy` 和 `Clone`。
- 需要共享资源：使用 `Rc`、`Arc` 或其他明确的共享所有权机制。
- 需要离开作用域时执行动作：实现 `Drop`，不要试图让它成为 `Copy`。

最终可以归纳为：

```text
Move：转移同一个所有权
Copy：无条件、隐式地复制普通值
Clone：显式创建新的所有权或共享关系
Drop：结束一份所有权并执行清理
```

`Clone` 能在复制过程中建立新的资源关系，所以可以和 `Drop` 配合；`Copy`
没有机会执行这些逻辑，所以不能和 `Drop` 同时存在。

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
