# 总结一下常见的原子指令实现，希望可以理解原子执行设计有什么考虑

参考 [OSTEP](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf)

1. Test-And-Set
1. Fetch-And-Add
2. Load-Linked and Store-Conditional
3. Compare-And-Swap

这里对比了下 ARM 从 ll-sc 到 fetch-and-add 的之后，似乎性能有较大的提升:
https://cpufun.substack.com/p/atomics-in-aarch64
