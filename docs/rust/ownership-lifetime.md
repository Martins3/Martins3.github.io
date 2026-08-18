## Ownership and lifetime

<!-- 9621368f-b648-41e2-ba4a-04a08a94a654 -->

[这个](https://www.reddit.com/r/rust/comments/mgh9n9/ownership_concept_diagram/)
对于 ownership 的总结，实际上，技术就是 Rc 和 Cell

https://github.com/usagi/rust-memory-container-cs/blob/master/3840x2160/rust-memory-container-cs-3840x2160-dark-back.png

所以，Cell 的到底是怎么实现的呀?

- [ ] Cell 增加了代码进行检查，那么到底检查的内容是什么 ?

看看这个教程了: https://github.com/tfpk/lifetimekata

borrow -> cell clone -> rc

## 检查工具

https://github.com/cordx56/rustowl

## FnOnce:只能调用一次的闭包
<!-- 41ad1270-6e36-4591-9f53-baf6e695bef3 -->

参考 [闭包](https://stevedonovan.github.io/rustifications/2018/08/18/rust-closures-are-hard.html)

闭包本质是匿名 struct,捕获的变量成为其字段。根据**如何捕获**,Rust 把闭包分成三个
trait:

| trait    | 捕获方式     | 调用次数                |
| -------- | ------------ | ----------------------- |
| `FnOnce` | 按值(所有权) | 一次,调用即消耗闭包本身 |
| `FnMut`  | 可变借用     | 多次,允许修改捕获的变量 |
| `Fn`     | 不可变借用   | 多次                    |

三者是继承关系:`Fn: FnMut: FnOnce`。即一个 `Fn` 闭包同时满足 `FnMut` 和 `FnOnce`
约束。所以接受 `F: FnOnce` 的泛型函数可以接收任何闭包,这也是为什么很多 API(如
`thread::spawn`、`Option::unwrap_or_else`)都用 `FnOnce` 约束——最宽松。

### 为什么 FnOnce 只能调用一次

闭包按值捕获后,捕获的变量就是闭包自己的字段。调用闭包时,`FnOnce` 的 trait 方法
`call_once(self)` 按值接收 `self`,闭包连同其捕获的所有权一起被消耗:

```rust
let s = String::from("hello");
let f = move || println!("{}", s); // 捕获 s 的所有权
f();
// f(); // error: use of moved value
```

### 典型场景

**1. `Option::take` + `unwrap`:把一次性回调塞进 Option**

`FnOnce` 调用即消耗自身,无法像 `&Fn` 那样反复借用调用。所以存 `FnOnce`
的经典模式是放进 `Option`,需要时用 `Option::take` 把所有权拿出来调用一次:

```rust
let mut opt = Some(|| println!("one-shot"));
let cb = opt.take().unwrap(); // 所有权拿出来
cb();
assert!(opt.is_none()); // 之后是 None,不可能再调用
```

**2. `Box<dyn FnOnce>`:把闭包装箱擦除类型**

`dyn FnOnce` 作为 trait object 时,调用同样消耗自身,只能调用一次:

```rust
let b: Box<dyn FnOnce()> = Box::new(|| println!("boxed"));
b();
// b(); // error: use of moved value
```

**3. `thread::spawn`:move 闭包跨线程搬运所有权**

`std::thread::spawn` 要求闭包满足 `FnOnce + Send + 'static`。`move`
把捕获变量所有搬运进新线程,新线程只能执行一次这个闭包,正好对应 `FnOnce`:

```rust
let msg = String::from("hi");
let h = std::thread::spawn(move || println!("{}", msg));
h.join().unwrap();
```

### 与所有权的关系

- **捕获 = 结构化 move**:闭包捕获变量相当于把变量移进匿名 struct,`move` 关键字只是强制按值捕获
- **`FnOnce` 调用 = 消费闭包**:`call_once(self)` 按值拿 `self`,和 `fn drop<T>(_: T)` 消费参数是同一个道理——所有权用一次就没了
- 判断一个闭包是哪个 trait:看捕获的变量是否被
  move、是否被修改。只有按值捕获(或内部 move 了捕获值)才是纯 `FnOnce`

## rust 生命周期描述符
<!-- d62b192f-258c-46b9-b7b6-3daf8b5adcf7 -->

对于第二条，问题是，如果 self 的声明周期 和 其他参数不一致会怎样 ?
代码的逻辑的逻辑会被检查
```rust
struct Name<'a> {
    x: &'a str,
    y: &'a str,
}

impl<'a> Name<'a> {
    fn longest(&self, y: &str) -> &str {
        if self.x.len() > y.len() {
            self.x
        } else {
            self.y
        }
    }
}
```


### 为什么时候使用 lifetime
具体来说：

1. 引用本身（最常见）
    - &'a str、&'a mut T 是最直接的场景，demo 里的 longest<'a> 就是。

2. 包含引用的类型
    - 结构体字段存引用（RefHolder<'a>）、枚举存引用、Cow<'a, str>、&'a [T] 切片等，都需要把 lifetime 传进去，因为类型要"记住"内部引
      用何时失效。

3. trait 对象
    - Box<dyn Trait + 'a>、&'a dyn Trait。这里的 'a 不是引用本身，而是说"这个 trait 对象内部可能借用数据，借用不能超过 'a"。

4. 'static 特例
    - &'static str 表示数据在整个程序运行期间都有效（字面量、常量），可以安全地存到全局、塞进 Box::leak 等。

5. 省略规则（elision）掩盖了大部分情况
    - 大多数时候你不需要写 'a，编译器按规则自动补上（只有一个输入引用、&self 方法等）。写不出来的时候（比如 longest），恰恰说明借用
      关系复杂到省略规则无法表达，必须显式标注。

所以本质是：lifetime 是编译期用来检查"引用不会比它指向的数据活得更久"的工具。凡是编译器无法自动推导出引用有效期的场合（多输入、类型
内部存引用、trait 对象），就需要显式写出 lifetime。demo 里的三个例子正好覆盖了这三类。


函数声明 lifetime elision 的原则, 为了解决函数返回值生命周期
1. 没有返回值，可以省略
2. 有 `&self` `&mut self` 可以省略，因为按照 self
3. 参数的 lifetime 都是一致的

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
