https://lwn.net/Articles/799218/

## rcu 的邮件，每一个都需要阅读下
- https://lore.kernel.org/rcu/E73E4593-DFA7-4A46-924C-3867CC0B4807@gmail.com/T/#m74d0caa2ea47bfc8fe369d18f772ca1244616d9a
  - 这里的 LKMM 是什么含义?
  - 里面的小测试都搞一下

也许最终可以来帮助 Paul 来 review patch 吧

## 为什么内核有定义了一个 LKMM ?
https://mp.weixin.qq.com/s/ooNq32HCF4PmKoirsczDtg

## 资源
- 工具链：`herdtools7` — 包含 `herd7` (模拟器) 和 `klitmus7` (硬件测试生成器)

https://pauillac.inria.fr/~maranget/papers/asplos2018.pdf

全序关系 如何理解?
Happens-Before

**From-Reads (fr)**
**Reads-From (rf)**
有区别？

cat 语言?

- [herdtools7 GitHub](https://github.com/herd/herdtools7)
- [diy7 文档](https://diy.inria.fr/doc/index.html)

- [ASPLOS 2017 Memory Model Verification](https://research.nvidia.com/sites/default/files/pubs/2017-04_Automated-Synthesis-of/ASPLOS_2017_Memory_Model_Verification.pdf)


- [What every systems programmer should know about concurrency](https://www.cl.cam.ac.uk/~pes20/weakmemory/cacm.pdf)
- [Weak vs Strong Memory Models](https://preshing.com/20120930/weak-vs-strong-memory-models/)
- [Memory Barriers Are Like Source Control Operations](https://preshing.com/20120710/memory-barriers-are-like-source-control-operations/)


- [LWN: Who ordered memory fences on an x86?](https://lwn.net/Articles/720550/)
- [LWN: Memory models](https://lwn.net/Articles/470681/)
- [Write once, herd everywhere (LPC)](https://lpc.events/event/7/contributions/653/attachments/605/1087/Write_once_herd_everywhere.pdf)
- [Memory Barriers: a Hardware View for Software Hackers](http://www.rdrop.com/users/paulmck/scalability/paper/whymb.2010.07.23a.pdf)
- [perfbook](https://git.kernel.org/pub/scm/linux/kernel/git/paulmck/perfbook.git)


- [内核 tools/memory-model/README](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/tools/memory-model/README)
- [内核 Documentation/litmus-tests.txt](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/litmus-tests.txt)


linux-drm/tools/testing/selftests/membarrier/ 这是很小的测试，按道理可以吸收进来

还有几个工具:
**MemAlloy**: 用于验证和比较内存模型的 Alloy 模型
**Nemos**: 另一个内存模型工具
**rmem**: ARM 的内存模型探索工具

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
