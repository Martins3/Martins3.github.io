# iridium

这个人写了一系列的教程:
https://blog.subnetzero.io/post/building-language-vm-part-24/

但是个人认为写的并不好:
- 加入了各种莫名奇妙的 cluster 和 network 的功能
- 废话非常多，而实际上关键的代码感觉也就 1000 行了
- 没有实现从高级语言的，而是汇编的

# 问题
- [ ] 需要处理分支预测?
- [ ] wren 需要 LLVM 吗?
- [ ] `parse_hex` 中的 rust 语法需要分析一下
- `#[macro_use]` 是什么意思
- https://blog.subnetzero.io/post/building-language-vm-part-08/
  - 分析 src/assembler/register_parsers.rs
    - [ ] 不过，我们首先需要学习一下 rust 的 macro
- [ ] 关于 nom 的问题
- [ ] 正是无法理解的 src/lib.rs
  - [ ] 为什么要单独创建一个这个文件出来
  - [ ] 什么是 extern
