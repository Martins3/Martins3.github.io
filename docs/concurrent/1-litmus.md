# Linux 内核 Litmus Tests 介绍
Documentation/litmus-tests/


## 先把基本的理论搞清楚了
- https://diy.inria.fr/doc/index.html

- https://github.com/herd/herdtools7/

- http://diy.inria.fr/

> We provide the following tools:
> - herd7: a generic simulator for weak memory models
> - litmus7: run litmus tests (given as assembler programs for Power, ARM, AArch64 or X86) to test the memory model of the executing machine
> - diy7: produce litmus tests from concise specifications

opam 环境初始化:
```sh
sudo yum install opam
sudo opam init
```

```sh
opam install herdtools7
```

## 也许这两个问题都是需要理解的
https://acg.cis.upenn.edu/papers/dac11_litmus.pdf
https://research.nvidia.com/sites/default/files/pubs/2017-04_Automated-Synthesis-of/ASPLOS_2017_Memory_Model_Verification.pdf
https://arxiv.org/pdf/2310.12337

https://github.com/litmus-tests/litmus-tests-riscv

- https://community.arm.com/arm-community-blogs/b/architectures-and-processors-blog/posts/generate-litmus-tests-automatically-diy7-tool


## 需要走的教程
- https://lwn.net/Articles/720550/
- https://lwn.net/Articles/470681/

有趣的报告:
https://lpc.events/event/7/contributions/653/attachments/605/1087/Write_once_herd_everywhere.pdf

## 不装逼，先把教程看完就可以了
https://git.kernel.org/pub/scm/linux/kernel/git/paulmck/perfbook.git

## kernel : tools/memory-model/Documentation/
```txt
herd7 -conf linux-kernel.cfg litmus-tests/MP+pooncerelease+poacquireonce.litmus
```
的确，很好玩，把所有的路线都走一遍。

为什么需要 0:r2 的前面
```txt
26 exists (0:r2=0 /\ 1:r4=0)
```

## 其他的地方的探索

写的很长 https://stackoverflow.com/questions/69112020/reason-for-the-name-of-the-store-buffer-litmus-test-on-x86-tso-memory-model

## 这个写的太好了
tools/memory-model/Documentation/README

还照顾到各个水平层次的了

## 工具
- https://github.com/herd/herdtools7

- 内核自带的 memory testing 工具:

- tools/testing/selftests/membarrier/

## 看看
https://news.ycombinator.com/item?id=44036829


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
