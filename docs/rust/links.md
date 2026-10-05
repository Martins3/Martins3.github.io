# Resource
- https://github.com/TheAlgorithms/Rust : 这就是入口了，使用这个去做 leetcode
  - https://github.com/EbTech/rust-algorithms : 看完之后，去刷题

https://rtpg.co/2020/12/11/dbg-in-python.html
http://dtrace.org/blogs/bmc/2020/10/11/rust-after-the-honeymoon/

[All](https://github.com/rust-unofficial/awesome-rust#resources)

https://www.ihcblog.com/rust-runtime-design-1/ : rust 的异步 io 模式。

[project base learning](https://github.com/tuvtran/project-based-learning#rust) 其中关于链表的很有意思。

https://anssi-fr.github.io/rust-guide/

https://blog.yoshuawuyts.com/state-machines/ : 博客

https://github.com/phil-opp/blog_os : 使用这个作为项目的基础也是不错的哦 !

https://blog.m-ou.se/writing-python-inside-rust-1/ : 相同的框架，为什么别人的blog 就是这样的

https://zhuanlan.zhihu.com/p/146472398 : 学习路线，其实体有意思的


https://github.com/ctjhoa/rust-learning : 各种教程汇总

https://github.com/rust-embedded/awesome-embedded-rust : 其实对于嵌入式并没有什么兴趣
https://lfn3.net/2020/08/03/a-gentle-intro-to-assembly-with-rust/ : Rust with assembly

https://rtpg.co/2020/12/11/dbg-in-python.html

https://fasterthanli.me/articles/a-half-hour-to-learn-rust : 半小时学习 rust

https://nick.groenen.me/posts/rust-error-handling/ : 一个人阅读 the rust book 之后开始写项目之后的感觉， error handling 的确很烦人。

https://github.com/ferrous-systems/elements-of-rust#combating-rightward-pressure : 一些常用写法 和 建议
https://github.com/fishinabarrel/linux-kernel-module-rust : 使用 Rust 来写 kernel module

#### What is the relation with str and String
https://mgattozzi.github.io/2016/05/26/how-do-i-str-string.html

#### Modular
https://doc.rust-lang.org/stable/rust-by-example/mod/split.html

#### Faq for Leetcode
1. [Append to vector as value of hashmap](https://stackoverflow.com/questions/33243784/append-to-vector-as-value-of-hashmap/33243862)
2. [Return a local hashmap](https://stackoverflow.com/questions/32682876/is-there-any-way-to-return-a-reference-to-a-variable-created-in-a-function)
3. [How to return a ref](https://bryce.fisher-fleig.org/blog/strategies-for-returning-references-in-rust/index.html)

4. https://github.com/aylei/leetcode-rust


#### This is fun
3. https://blog.subnetzero.io/post/building-language-vm-part-00/
4. https://vnduongthanhtung.gitbooks.io/migrate-from-c-to-rust/content/string-manipulations.html




## projects
- https://www.philippflenker.com/hecto-chapter-6/ : 使用 rust 写编辑器的教程

## fun
https://github.com/adamsky/globe
https://github.com/rust-unofficial/patterns
http://technosophos.com/2019/08/07/writing-a-kubernetes-controller-in-rust.html
https://www.osohq.com/post/rust-reflection-pt-1
https://github.com/ingraind/redbpf : Rust 提供给 bpf 的接口，但是我没有办法让 cargo 在 Sudo 下运行。

## blog
- https://www.brandons.me/blog/why-rust-strings-seem-hard : 介绍 Rust 的 String 使用

## TODO
4. use rust::blog::Post; 为什么需要添加rust:: 来指示本地包的作用 ?
6. 我们可以在返回值中间包含mut keyword 吗 ?
7. https://doc.rust-lang.org/rust-by-example/std/hash.html `literal string`为什么总是在添加&使用

因此，Rust 类型系统和 *trait bound* 确保永远也不会意外的将不安全的 Rc<T> 在线程间发送

8. 思考一个问题：
  1. 当一个函数的作用是返回一个 struct, 进而这一个 struct 需要在在各个函数之间传递，如何保证该结构体生命周期的正确性。
  2. 或者说，在 c++ 中间，在一个函数中间 new 了一个对象，之后在任何地方 delete 掉，如何处理
  3. 甚至更加过分一点，一个 thread 创建了一个对象，但是这个对象需要被其他的 thread 使用，如何 ?

## 泛型
1. https://www.reddit.com/r/rust/comments/7llmu1/why_do_you_need_to_declare_generic_twice_in_impl/
2. 如何让泛型变得难以理解:
    1. 多个泛型类型 T U
    2. 对于泛型类型添加限制
    3. 添加生命周期



## 函数式
1. https://stackoverflow.com/questions/34733811/what-is-the-difference-between-iter-and-into-iter
2. https://danielkeep.github.io/itercheat_baked.html
3. https://doc.rust-lang.org/rust-by-example/fn/closures/capture.html 解释为什么有的closure 需要  mut

## 从最简单的问题分析起
为什么在Rust中间创建一个链表如此麻烦:
https://news.ycombinator.com/item?id=16442743


## Effective Rust
https://news.ycombinator.com/item?id=36338529

## 又一个教程合集
https://www.arewewebyet.org/

## 框架
https://github.com/crossbeam-rs/crossbeam

https://github.com/zjp-CN/tlborm


https://github.com/sunface/rust-by-practice

https://zh.practice.rs/why-exercise.html

https://github.com/sunface/too-many-lists

https://github.com/HigherOrderCO/HVM

http://www.cmyr.net/blog/keypaths.html

https://github.com/TheAlgorithms/Rust

https://www.lpalmieri.com/posts/error-handling-rust/?utm_campaign=Book&utm_source=Reddit&utm_medium=Social

## checksheet
https://cheats.rs/#generics-constraints

这个的确是极好的

https://github.com/rust-boom/rust-boom : 教程合集

## rust
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


https://github.com/denoland/celld : hn 上看到的。。。

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
