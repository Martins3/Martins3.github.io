# mttcg
:WIP:

- [ ] 参考资料 : https://research.swtch.com/mm
- [ ] io_uring 为什么需要 smp_mb
- [ ] 量化中的教程
- [ ] 内核中的经典教程
- [ ] 分析为什么在二进制翻译中存在挑战的
- [ ] 之前的华为的形式化验证是不是就是处理内存序列的 ，把那个文章拿过来读读开开眼吧!

## mttcg
https://lwn.net/Articles/697265/

在 A TCG primer 很好的总结了 TCG 的工作模式以及退出的原因。

在 Atomics,
Save load value/address and check on store,
Load-link/store-conditional instruction support via SoftMMU,
Link helpers and instrumented stores,
中应该是分析了在 TCG 需要增加的工作

在 Memory coherency 分析的东西，暂时有点迷茫，不知道想要表达什么东西。 // TODO

## mttcg
[^1] 指出
1. 如果需要支持 icount 机制将会消失
2. 想要让一个 guest 架构支持 mttcg 需要完成的工作
3. Enabling strong-on-weak memory consistency (e.g. emulate x86 on an ARM host)


[^1]: https://wiki.qemu.org/Features/tcg-multithread
[^2]: https://qemu-project.gitlab.io/qemu/devel/multi-thread-tcg.html?highlight=bql
[^6]: https://lwn.net/Articles/517475/
[^7]: https://qemu.readthedocs.io/en/latest/devel/multi-thread-tcg.html
