## rust 中的 macro
<!-- e91004e0-b336-4d47-98b8-357476de7e67 -->

https://github.com/dtolnay/proc-macro-workshop

? macro 的定义非常奇怪，模式后面的代码如果想要定义 statement 需要含有两层 { { .}}

However, unlike macros in C and other languages, **Rust macros are expanded into abstract syntax trees**, rather than string preprocessing,
so you don't get unexpected precedence bugs.

there are three basic ideas for macro:
    1. Pattern and Designator
    2. overloading
    3. repetition

Rust 中主要有三种宏机制:

| 类型                              | 定义方式                                                             | 用途                         | 特点                                                     |
| ---------------------------       | -------------------------------------------------------------------- | --------------------         | ---------------------------------------                  |
| **Declarative Macro**（声明式宏） | `macro_rules!`                                                       | 通过模式匹配生成代码         | 类似于“宏模板”，可匹配复杂语法模式                       |
| **Procedural Macro**（过程宏）    | `#[proc_macro]` / `#[proc_macro_derive]` / `#[proc_macro_attribute]` | 通过 Rust 代码生成 Rust 代码 | 更灵活，可操作 TokenStream，支持自定义 `derive` 或属性宏 |
| **内置宏**                        | `println!`, `format!` 等                                             | 系统提供的方便宏             | 通常是声明式宏或过程宏的封装                             |


声明式宏（macro_rules!）
- 用户写 macro_rules! foo { ... }
- 编译器在遇到 foo!(...) 时，将输入与宏规则模式匹配。
- 匹配成功后，将模板展开成 Rust 代码。
- 展开的代码再进入正常的编译流程。


相比 macro_rules!，过程宏更灵活，因为它操作语法树。其工作流程
- Rust 将宏调用参数转换成 TokenStream（类似语法树的一维表示）。
- 宏函数（用 Rust 写）处理这个 TokenStream。
- 宏返回另一个 TokenStream。
- 编译器将返回的 TokenStream 当作 Rust 代码编译。


### 宏
1. 在附录 C 中会探讨 derive 属性，其生成各种 trait 的实现。
2. 宏和函数的最后一个重要的区别是：在调用宏 之前 必须定义并将其引入作用域，而函数则可以在任何地方定义和调用
5. 无论何时导入定义了宏的包，#[macro_export] 注解说明宏应该是可用的。 如果没有该注解，这个宏不能被引入作用域。

## 为什么 vec 和 println 是 macro

主要是为了考虑可变参数吧

## 其他资料
- https://github.com/wdanilo/crabtime : 库
- https://github.com/tfpk/macrokata
- https://github.com/DanielKeep/tlborm : 11 年前更新


https://www.reddit.com/r/rust/comments/1qon5p9/media_crabtime_a_novel_way_to_write_rust_macros/

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
