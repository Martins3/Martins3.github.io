# What's CFI in kernel

阅读 LWN 的时候[^1]，遇到 CFI 相关的文章，这个东西多次骚扰过我，忍不了。

## TODO
- [ ] 在汇编中看到了很多 cfi 的 macro 啊
- [ ] 为什么总是间接跳转被针对，直接跳转无法处理吗?
- [ ] 似乎 cfi 是因为 LLVM 的指令流分析得出来的结果，如果其实根本分析不出来，怎么办?
  - [ ] 如果 cfi 分析，是存在多个可能结果，如何办?
- [ ] CFI 在 kernel 上有什么特殊的地方吗?

## 阅读，随便找点文章读读

### https://www.redhat.com/en/blog/fighting-exploits-control-flow-integrity-cfi-clang

- return oriented programming 是常见的利用的方法 TODO what's ?
    - https://en.wikipedia.org/wiki/Return-oriented_programming


Calls to virtual functions or class casting in software that were written in C++, as an example,
can only be determined during execution.
For these cases clang relies on LTO (link-time optimization) information.
To compile a code with CFI support one necessarily needs to compile it with LTO enabled.

TODO 什么是 LTO

Currently, for `x86_64` architecture, LLVM can only validate forward-edge control flow, thus function return (backward-edge) is not checked. Given CFI requires the software to be compiled with the LTO option, this may cause some issues when compiling software linked with shared libraries in some cases.

TODO
1. 什么是 forward-edge control flow
2. 为什么无法支持

## [ ] https://lwn.net/Articles/898040/

## [ ] https://lwn.net/Articles/856514/

[^1]: https://lwn.net/Articles/898157/bigpage
