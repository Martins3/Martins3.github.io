# 淦，设计一个裸金属二进制翻译器不可能这么难

> 一件事最可怕的时刻，总是在你开始做之前。[^5]

## 跌宕起伏
三年前（2019 ）的夏天，那个时候我还在尝试理解 mmap(2) 中的 anonymous  mapping 是什么意思。
老板提到，Transmeta 曾在 VLIW CPU 上面直接运行二进制翻译器模拟 x86 指令集，性能甚至比同时期的 X86 CPU 性能更高。

然后我去国科大怀柔校区上研究生一年级，哪里上了很多分散的课程，顺便看看了 Linux 内核。
研究生二年级的时候写了一个 [Loongson Dune](https://github.com/Martins3/loongson-dune)，通过 Loongson Dune 可以将一个进程放到虚拟机中运行，
代码量不到 2000 行，很有意思。

在 2021/4 的时候，我处于无事可做的状态，因为之前看了一些 Linux 内核的东西，所以总是想写点系统态的程序，然后就和老板说，写个裸金属二进制翻译器如何。

具体怎么写还没有思路，所以调查各种一下 hypervisor ，主要是:
- Captive [^2]
- Transmeta [^3]
- ramooflax [^1]

当时还有很多不成熟的想法:
- 让 Guest 代码在 non-root 模式下运行；
- 让 Guest 使用 virtio 驱动，从头构建设备驱动；
- 将 Linux 作为一个 Unikerne，然后在上面运行 QEMU。

分析了一下 Captive 之后，和老板讨论了一下如何处理设备的问题，老板说：”按照我的理解，设备是可以直通的“。
从此，我豁然开朗，之后的一年的时间里，技术路线几乎没有发生任何变化，而且任何进展都是符合预期的。

到了 7 月份，这个时候已经开始秋招，虽然这个项目只是开始中，还是写到简历上了，和各路面试官交流，因为几乎没有人知道二进制翻译器是做什么的。
将一个安装了 BMBT 的 U 盘插到一个 LoongArch 电脑上，然后开机，这台电脑就变为一个 X86 电脑，这件事情这么酷绚，确无人理解，让我感觉有点郁闷。

到了 8 月份，给胡伟武老师做报告，介绍了一下 BMBT 的设计和实现思路，
其他的同学都是很认真的准备的，我就是[随便搞搞](https://martins3.github.io/slides/repo/2021-8-24/index.html)，别人问，我就说，没时间了，代码写不完了。
胡老师问:"你列举了这么多的事项，怎么都是待办啊"，然后大家都笑起来了，老板连忙解释说这里工程量很大。

到了 2021/10/25 时候，代码终于可以链接了，虽然很多函数还只是空的实现。

到 2021/12/1 的时候，这个时候，终于可以运行到 seabios 的入口。
这个时候，我才发现从一开始恐惧我的 SMBIOS 以及 acpi 都是可以被绕过的，也就是从这个时候开始，我有一个感觉，那就是这个项目是可以写完的。

然后花费了 20 天左右熟悉了一下 UEFI ，因为之前老板总是感觉可以让 BMBT 作为 UEFI 的 driver 的，这样可以利用 UEFI 中的 libc
但是等我理解了 UEFI 的大致工作原理之后，这条路是走不通的，因为 BMBT 需要直接掌管所有的设备，而 UEFI
还是封装了这些设备的访问，而且 UEFI 的 libc 的质量相当的差。

发现 UEFI 的路线走不通之后，那就没有什么好办法了，还是需要构建一个裸机的 C 库，实际上，基于 musl 构建相当简单。
在构建裸机 C 库的过程中，发现自己曾经看的《程序员的自我修养》还是相当有趣的。

到了 2022/1/22 的时候，终于可以在 KVM 中运行了，其中搭建 KVM 的运行环境，还是花费了不少时间的。
老板调侃到，终于可以过一个好年。

过年之后的时间，我就没有搞任何 BMBT 的事情，中间花费了一点时间整理之前 Hacker News 中的阅读笔记，
然后重构了自己的 vim 配置，等到重新着手工作，就已经快到 3 月份，确认了一下 VKM 时钟中断的 bug，然后同步了一下 LATX 二进制翻译器引擎。

串口直通和 PCI 设备直通搞完就是已经到了 4 月初，但是 4 月 28 日就需要交论文。
之前没有任何在裸机上调试的经验，和 firmware 工程师交流，他们表示从 kvm 到裸机上是需要不少功夫的，
好家伙，我这就开始慌了。

然后全村老小一起出动，帮我搞了一台调试的机器，一个串口线和一个 ejtag 调试器，后来发现从来都没有使用过
ejtag 调试器，一直都是使用串口就差不多了，每次都是以为自己成功了，终于可以结束了，最后总是发现其实还是有一些
问题，每天的心情都像是做过山车一样的。
- :neutral_face: 发现无法正常输出，就是一行乱码。
- :star_struck: 从 LoongArch 内核中抄过来串口输出的代码，然后就可以了。
- :neutral_face: 但是在 seabios 中发现了报错。
- :star_struck: 结果发现是 seabios 自己的 bug，没有考虑到物理内存可能没有正确初始化。
- :neutral_face: 运行到 shell 中，但是发现有的 PCIe 设备没有找到。
- :star_struck: 原来是 kvm 中没有正确模拟 PCIe bridge，在 PCIe bridge 下的设备需要考虑 PCIe bridge window 才可以被正确扫描。
- :neutral_face: 发现 shell 的交互太慢了，而且串口中断根本没有被 Guest 接受过。
- :star_struck: 原来是中断控制器没有正确的初始化，kvm 中和物理机的中断控制器配置不相同。
- :neutral_face: 结果测性能发现数据明显不对。
- :star_struck: 发现是因为 hpet 设备没有实现，导致 Guest 内核使用 jiffies 来计算时钟，那个时钟在高负载的时候不准，通过 hacking Guest 内核可以解决。

## 感触
最开始时候，似乎这个项目似乎是准备当作博士课题的。
和老板商量的时候，准备作为两个硕士来写，但是另一个人一直在忙别的项目，
慢慢的就变成我一个人的在搞了。项目写到中期的时候，和学弟讨论起来，我还打趣到，实在不行就靠 Loongson Dune 毕业了。

说实话，现在看来，似乎这个项目也没有那么难，甚至有时候，我都在反思，为什么要写那么长的时间。

在很多地方，没有动手之前，非常恐惧，但是一旦深入进去，发现也不过如此:
- ACPI / SMBIOS : 不支持, 其实也是勉强可以使用的。
- musl 库 : 被渲染的很难，但是其实也是 copy and paste 的过程，大约一周就可以移植成功。
- 在裸机上的调试 : 最后发现根本不需要什么特殊的技术，一个 printf 几乎可以解决所有的问题。
- grub 加载 : 发现只要将 bmbt 编译为一个 elf 就可以，然后不需要任何修改就可以被 QEMU 和硬件加载
- [ ] 多线程 : 虽然暂时 BMBT 不支持，不过我推测不会很难。

总体来说，这个项目体验还是不错的，代码量很大，什么都触碰了一遍，从 QEMU ，内核，到裸机环境的调试，
而且还是有一点微小的创新的。

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
[^1]: [ramooflax](https://github.com/airbus-seclab/ramooflax)
[^2]: [The Transmeta Code Morphing Software: Using Speculation, Recovery, and Adaptive Retranslation to Address Real-Life Challenges](https://safari.ethz.ch/digitaltechnik/spring2019/lib/exe/fetch.php?media=dehnert_transmeta_code_morphing_software.pdf)
[^3]: [Captive](https://www.usenix.org/system/files/atc19-spink.pdf)
[^4]: [A Linux in Unikernel Clothing](https://dl.acm.org/doi/pdf/10.1145/3342195.3387526)
[^5]: [According to Stephen King: "The Scariest Moment Is Always \_\_\__."](https://gretchenrubin.com/2016/08/according-stephen-king-scariest-moment-always-____)
