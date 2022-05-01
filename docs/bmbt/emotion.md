# 淦，设计一个裸金属二进制翻译器不可能这么难

三年前（2019 ）的夏天，那个时候我还在尝试理解 mmap(2) 中的 anonymous  mapping 是什么意思
老板提到 Transmeta 曾经作出 VLIW CPU 上面直接运行二进制翻译器模拟 x86 指令集，性能甚至比同时期的 X86 CPU 性能更高。

然后我去国科大怀柔校区上一年级，哪里上了很多分散的课程，顺便看看了 Linux 内核。
研究生二年级的时候写了一个 [Loongson Dune](https://github.com/Martins3/loongson-dune)，通过 Loongson Dune 可以将一个进程放到虚拟机中运行，
代码量不到 2000 行，很有意思。在 2021/4 的时候，我处于无事可做的状态，因为之前看了一些 Linux 内核的东西，所以总是想写点系统态中程序，
然后就和老板说，写个裸金属二进制翻译器如何。

具体怎么写还没有思路，所以调查各种一下 hypervisor ，主要是:
- [Captive](.)
- [Transmeta](.)
- [ramooflax](https://github.com/airbus-seclab/ramooflax)


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
