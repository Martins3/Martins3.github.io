## 教程
- [Asynchronous Programming in Rust](https://github.com/rust-lang/async-book)
- [RustBook](https://github.com/QMHTMY/RustBook) : 中文的 rust 的书籍
- [Comprehensive Rust 🦀](https://google.github.io/comprehensive-rust/structs.html)

## 文摘
- [Why not Rust](https://matklad.github.io//2020/09/20/why-not-rust.html) : 指出 Rust 的一些问题，关于 C++ 和 Rust 的性能对比，理解很深刻。
- [A Rust tutorial for experienced C and C++ programmers](https://github.com/nrc/r4cppp) : 比较简短的对比 cpp 和 Rust，最后分析 graph 如何实现。
  - [A guide to porting c to rust](https://locka99.gitbooks.io/a-guide-to-porting-c-to-rust/content/)
  - [Thread Safety in C++ and Rust](https://blog.reverberate.org/2021/12/18/thread-safety-cpp-rust.html)
- [From Julia to Rust](https://miguelraz.github.io/blog/juliatorust/) : 虽然是分析从 Julia 的过来人如何写 Rust，但是实际上携带了很多资源
  - [ ] https://cheats.rs/
  - [Coz: Finding Code that Counts with Causal Profiling](https://github.com/plasma-umass/coz)
  - [The egg project uses e-graphs to provide a new way to build program optimizers and synthesizers.](https://egraphs-good.github.io/)
  - [straight : A model checker for implementing distributed systems.](https://github.com/stateright/stateright)
  - [gleam: A friendly language for building type-safe, scalable systems!](https://github.com/gleam-lang/gleam) : rust 实现的语言
- [10 万行 rust 之后的经验](https://matklad.github.io/2021/09/05/Rust100k.html)
- [RustMagazine 中文月刊](https://rustmagazine.github.io/rust_magazine_2021/chapter_12/toc.html) : 国内写的一些文摘
- [Making slow Rust code fast](https://patrickfreed.github.io/rust/2021/10/15/making-slow-rust-code-fast.html#viewing-criterions-html-report) : 如何在 Rust 上搞性能分析
- [pretzelhammer's Rust blog](https://github.com/pretzelhammer/rust-blog) : 不能理解为什么有 4.1k 的 star
- [迷思](https://zhuanlan.zhihu.com/prattle) : 从 rust 的角度分析计算机网络，安全等

## 项目
- [Aims to be compatible with the Linux ABI](https://github.com/nuta/kerla)
- [rustviz](https://github.com/rustviz/rustviz): Interactively Visualizing Ownership and Borrowing for Rust
- [bevy](https://github.com/bevyengine/bevy) : data-driven game engine
- [embassy](https://github.com/embassy-rs/embassy) : Modern embedded framework, using Rust and async.

## 资源
- [Langdev libraries for Rust](https://github.com/Kixiron/rust-langdev) : 使用 Rust 写编译器的一些开发工具
- [awesome-rust-cloud-native](https://github.com/awesome-rust-cloud-native/awesome-rust-cloud-native) : 收集 cloud native 中的 Rust 项目。

## 工具
- [cxx](https://github.com/dtolnay/cxx) : safe FFI between Rust and C++
- [pyo3](https://github.com/PyO3/pyo3) : Calling Rust from Python using PyO3
- [rust-clippy](https://github.com/rust-lang/rust-clippy) : 更加优雅的报错

## TODO
- pin : https://course.rs/advance/circle-self-ref/self-referential.html
- https://github.com/skyzh/type-exercise-in-rust
- https://stackoverflow.com/questions/37149831/what-is-the-difference-between-these-3-ways-of-declaring-a-string-in-rust
- https://github.com/nnethercote/perf-book : rust 的 perf book

https://kaisery.github.io/trpl-zh-cn/

https://www.lpalmieri.com/posts/error-handling-rust


## 参考这个教程阅读?
https://web.stanford.edu/class/cs110l/

https://www.shuttle.rs/blog/2024/04/18/using-traits-generics-rust

https://news.ycombinator.com/item?id=40385536

## 这个分析方法了解下

> cargo run --profile profiling --features profiling -- --no-vsync --no-multigrid

https://github.com/neovide/neovide/issues/2602

## 先看这个吧
https://github.com/pretzelhammer/rust-blog/blob/master/posts/learning-rust-in-2024.md


https://news.ycombinator.com/item?id=42219627


https://news.ycombinator.com/item?id=42274834

https://news.ycombinator.com/item?id=42280615

- https://news.ycombinator.com/item?id=42361793
  - https://github.com/skerkour/black-hat-rust

https://lore.kernel.org/lkml/20210414184604.23473-1-ojeda@kernel.org/

https://news.ycombinator.com/item?id=43340731

## 其实 python 的错误处理就一直没有搞懂
https://www.lpalmieri.com/posts/error-handling-rust

## 这个
https://news.ycombinator.com/item?id=43403821



## 形式化验证
https://news.ycombinator.com/item?id=43360633

https://news.ycombinator.com/item?id=42476192

## 他写了 perf book
https://github.com/nnethercote
https://nnethercote.github.io/perf-book/title-page.html

https://news.ycombinator.com/item?id=43978435

## 用这个案例去理解 rust 对于 c 的改变
https://news.ycombinator.com/item?id=44103116

## 这个非常好
https://news.ycombinator.com/item?id=45140572
- https://rustcurious.com/elements/ : 各种类型的 checksheet

- https://news.ycombinator.com/item?id=45826348
	- The state of SIMD in Rust in 2025

## 有趣

https://news.ycombinator.com/item?id=45973709

https://catcoding.me/p/rust-in-cloudflare-incident/

## 为什么需要这个项目?
https://github.com/microsoft/windows-rs

那么微软的 sdk 都是如何提供的?

https://kennykerr.ca/rust-getting-started/

## fil-c
Fil-C: completely compatible memory safety for C and C++

https://github.com/pizlonator/fil-c/

## 这里太不错了
https://dieterplex.github.io/rust-ebookshelf/


https://github.com/buyukakyuz/corroded


## 看看这两个，似乎可以动手操作
- https://blogsystem5.substack.com/p/ioctls-rust
- https://eta.st/2021/03/08/async-rust-2.html : 这个看看
- https://github.com/Azure/kimojio-rs : rust io uring


https://github.com/rizsotto/Bear : 已经全部修改为 rust 了
os/libos.md 中提到的 demikernel


https://github.com/criterion-rs/criterion.rs 好工具

学学这个
https://github.com/microsoft/bf-tree/agents?author=Martins3

## litebox
libos ，很好
https://github.com/microsoft/litebox

libos 和 microvm 都是有趣的东西

https://news.ycombinator.com/item?id=46913793

类似的这个东西:
https://github.com/containers/libkrun

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
