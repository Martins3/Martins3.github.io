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

[neovide](https://github.com/Kethku/neovide) neovim 客户端，只有 7000 行

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

- [闭包](https://stevedonovan.github.io/rustifications/2018/08/18/rust-closures-are-hard.html)


第十三章讲到如果可以在参数列表前使用 move 关键字强制闭包获取其使用的环境值的所有权.

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
