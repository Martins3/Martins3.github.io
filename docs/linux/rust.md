- `static` and `const` 的区别
  - https://stackoverflow.com/questions/52751597/what-is-the-difference-between-a-constant-and-a-static-variable-and-which-should

## 教程
- [The Little Book of Rust Macros](https://danielkeep.github.io/tlborm/book/README.html)
- [Rust Latam: procedural macros workshop](https://github.com/dtolnay/proc-macro-workshop) : 学一下 macro
- [Asynchronous Programming in Rust](https://github.com/rust-lang/async-book)
- [RustBook](https://github.com/QMHTMY/RustBook) : 中文的 rust 的书籍

## 文摘
- [Why not Rust](https://matklad.github.io//2020/09/20/why-not-rust.html) : 指出 Rust 的一些问题，关于 C++ 和 Rust 的性能对比，理解很深刻。
- [A Rust tutorial for experienced C and C++ programmers](https://github.com/nrc/r4cppp) : 比较简短的对比 cpp 和 Rust，最后分析 graph 如何实现。
  - [A guide to porting c to rust](https://locka99.gitbooks.io/a-guide-to-porting-c-to-rust/content/)
  - [Thread Safety in C++ and Rust](https://blog.reverberate.org/2021/12/18/thread-safety-cpp-rust.html)
- [From Julia to Rust](https://miguelraz.github.io/blog/juliatorust/) : 虽然是分析从 Julia 的过来人如何写 Rust，但是实际上携带了很多资源
  - [ ] https://cheats.rs/
  - [Coz: Finding Code that Counts with Causal Profiling](https://github.com/plasma-umass/coz)
  - [The egg project uses e-graphs to provide a new way to build program optimizers and synthesizers.](https://egraphs-good.github.io/)
  - [Loom is a testing tool for concurrent Rust code.](https://github.com/tokio-rs/loom)
  - [straight : A model checker for implementing distributed systems.](https://github.com/stateright/stateright)
  - [gleam: A friendly language for building type-safe, scalable systems!](https://github.com/gleam-lang/gleam) : rust 实现的语言
- [tokio 的源码分析](https://tony612.github.io/tokio-internals/01.html) : 中文的，很简短
- [10 万行 rust 之后的经验](https://matklad.github.io/2021/09/05/Rust100k.html)
- [RustMagazine 中文月刊](https://rustmagazine.github.io/rust_magazine_2021/chapter_12/toc.html) : 国内写的一些文摘
- [Making slow Rust code fast](https://patrickfreed.github.io/rust/2021/10/15/making-slow-rust-code-fast.html#viewing-criterions-html-report) : 如何在 Rust 上搞性能分析
- [pretzelhammer's Rust blog](https://github.com/pretzelhammer/rust-blog) : 不能理解为什么有 4.1k 的 star
- [迷思](https://zhuanlan.zhihu.com/prattle) : 从 rust 的角度分析计算机网络，安全等

## 项目
- [Aims to be compatible with the Linux ABI](https://github.com/nuta/kerla)
- [rustviz](https://github.com/rustviz/rustviz): Interactively Visualizing Ownership and Borrowing for Rust
- [bevy](https://github.com/bevyengine/bevy) : data-driven game engine
- [KVM firmware Rust 实现](https://news.ycombinator.com/item?id=19883626)

## 资源
- [Langdev libraries for Rust](https://github.com/Kixiron/rust-langdev) : 使用 Rust 写编译器的一些开发工具
- [awesome-rust-cloud-native](https://github.com/awesome-rust-cloud-native/awesome-rust-cloud-native) : 收集 cloud native 中的 Rust 项目。

## 工具
- [cxx](https://github.com/dtolnay/cxx) : safe FFI between Rust and C++
- [pyo3](https://github.com/PyO3/pyo3) : Calling Rust from Python using PyO3
- [rust-clippy](https://github.com/rust-lang/rust-clippy) : 更加优雅的报错

## kernel
- https://security.googleblog.com/2021/04/rust-in-linux-kernel.html

## TODO
- pin : https://course.rs/advance/circle-self-ref/self-referential.html
- https://github.com/skyzh/type-exercise-in-rust
- https://stackoverflow.com/questions/37149831/what-is-the-difference-between-these-3-ways-of-declaring-a-string-in-rust
