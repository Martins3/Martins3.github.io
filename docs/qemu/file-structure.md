# QEMU 概述

我个人的计算机学习习惯是，首先知道一个东西的大致是个什么东西，可以做什么，不可以做什么，以后有问题知道在哪里查询。
对于 QEMU 也是如此，本文首先简单地分析一下 QEMU 里面大概有什么。

QEMU 是虚拟化的集大成者，有的项目可以利用 kvm 构建虚拟机，例如 [firecracker](https://github.com/firecracker-microvm/firecracker)，但是
其只能使用 virtio IO 模拟设备[^4]，但是 QEMU 还可以使用设备模拟和直通的方法，有的项目，例如 Apple M1 Mac 上的 [Rosetta](https://en.wikipedia.org/wiki/Rosetta_(software))，但是其只能进行
x86 到 arm 上的二进制翻译器，而 QEMU 可以支持多达 20 个架构的互相翻译。

![](./img/qemu.svg)

## CPU
图上只是画了两个，是这两个我比较熟悉 :-) ，但是实际上还支持其他的一些技术，例如 xen 。

kvm 利用硬件加速，总体代码量很小，而且大多数代码都在 Linux 内核中。tcg 技术是将一个架构，例如 x86，的指令翻译为 tcg IR (Tiny Code Generator) (Intermediate Representation)，然后 tcg ir
翻译为另一个架构的指令，例如 arm 上。

根据指令是否可以在系统态中运行，可以将指令划分为 privileged 和 unprivileged，很多 privileged 指令的语义非常复杂，在 QEMU 中会调用对应的 helper 函数来实现，
这些 helper 在各个架构的  `target/$(arch)/tcg` 下。

kvm 不能跨架构，而利用 tcg 可以在 arm 上运行 x86 的操作系统，一般来说，kvm 要比 tcg 快 10 ~ 20 倍。

除了运行一个操作系统，tcg 还可以翻译一个用户态程序，例如在 x86 版本的微信运行在 arm 的操作系统上，这个功能 Rosetta 也可以做到，但是 Rosetta 也仅仅只能实现这个功能。

最后看一下相关功能的源码位置:

| cpu | source code location               |
|-----|------------------------------------|
| tcg | `accel` `tcg` `target/$(arch)/tcg` |
| kvm | `accel` `target/$(arch)/kvm`       |

`target/$(arch)` 下是各个架构相关的模拟，例如 target/i386/cpu.c 中就定义了 x86 的各种 CPU 模型，Tiger Lake, Haswell 之类的。
accel 中处理的是 CPU 模拟的通用结构，因为 tcg 下是各个架构指令翻译为 tcg IR，`target/$(arch)/tcg` 中是 tcg IR 翻译为各个架构指令的代码。
总体来说，内核在各个架构上提供的 kvm 接口比较统一，但是还是存在一定的差异，其差异的地方定义在 `target/$(arch)/kvm` 中。

## Device
QEMU 可以模拟的设备非常多，可以使用这个命令查询:
```sh
qemu-system-x86_64 -device help
```

如果查询 QEMU  nic 支持的模式可以使用这种方法:
```sh
qemu-system-x86_64 -net nic,model=help
```

QEMU 在设备模拟上采取了前端和后端分离的设计模式，使用 block 设备举个例子:
- Guest 如果需要访问 nvme 和 ide 设备，那么 QEMU 需要精确模拟这两个设备的行为，让 Guest 以为是在和真的 nvme 或者 ide 交互，这就是前端。
- 可以使用 Host 上的一个文件模拟存储设备，或者 /dev/nvme0n1 这种真正的 block 设备，而且这些设备不一定需要在本地，还可以在远程，这就是后端。

检查可以支持的后端的方法:
```sh
qemu-system-x86_64 -chardev help # 查询 QEMU 字符设备支持的后端
qemu-system-x86_64 -netdev help # 查询 QEMU 网络设备支持的后端
```

当然 QEMU 还支持 virtio 半虚拟化和 VFIO 设备直通，其代码也在 hw 下。

最后看一下相关功能的源码位置:

| device | source code location                           |
|--------|------------------------------------------------|
| block  | `./block` 后端模拟，`hw/block` 前端模拟        |
| char   | `./char`  后端模拟, `hw/char` 前端模拟         |
| net    | ./slirp 和 `./net` 后端模拟，`hw/net` 前端模拟 |
| video  | `hw/display`                                   |
| sound  | `./sound`                                      |
| arch   | `hw/$(arch)/` `hw/intc/`                       |

设备模拟的代码非常之多，我使用 cloc 统计了一下，大约有 50w 行，而 QEMU 总共才 100w 行。
对于大多数人，甚至很多专业的工程师，这些都是一些不需要阅读的内容。

## Memory
如果 Guest 访问了一个物理地址，是物理内存，还是 memory mapped io (mmio)，其行为是不同的，因为 QEMU 具有很强的灵活性，
设备是可以动态配置的，相同的物理地址，不同的硬件配置下，其行为不同，所以需要一个机制来确定这个物理地址到底是落到内存或者是那个设备的上，
这个机制就是 memory model 。

详细内容可以参考 [我的 blog](./memory.md)，其代码在 softmmu/physmem.c 中。

另一个和 memory 有关的是，在 tcg 模式下，因为没有硬件支持，所以从 GVA (Guest Virtual Address) 到 GPA(Guest Physical Address) 的翻译需要软件来做，这个就是 softtlb 中了。

详细内容可以参考 [我的另一篇 blog](./softmmu.md)，具体代码在 accel/tcg/cputlb.c 中 。

## Others
挑选一些其他有意思的目录说明一下:

| 辅助功能   | 描述                                                                                                                                                       |
|------------|------------------------------------------------------------------------------------------------------------------------------------------------------------|
| trace [^5] | 使用 ftrace 等工具对于 QEMU 进行 trace                                                                                                                     |
| fsdev      | 共享目录，可以使用 virtio fs[^1] 和 plan 9 来让 Guest 和 Host 共享目录                                                                                          |
| migration  | 热迁移虚拟机                                                                                                                                               |
| gdbstub    | 和 gdb 配合使用，可以调试 guest 内核                                                                                                                       |
| ndb        | 利用 Linux 内核的 ndb 功能，QEMU 可以用远程的 disk 来给 guest 模拟内核                                                                                     |
| disas      | 反汇编器，在 tcg 执行中，其实最开始需要将二进制反汇编成为 Guest 指令，然后才是让翻译器将 Guest 指令翻译为 Host 指令，QEMU 也可以使用 Capstone 来替换内置的 |
| ui         | QEMU 可以在终端中运行，也可以有 GUI 中，也甚至在 vnc 中运行，使用的底层的图形库支持 gtk 和 sdl2 等                                                         |

## Architecture

| 基础设备 | 描述                                                                         |
|----------|------------------------------------------------------------------------------|
| qdev     | QEMU 最开始的描述的设备层次结构的模型                                        |
| qom      | QEMU 进一步演化出来的面向对象模型                                            |
| qapi     | virsh 等工具和 QEMU 交互的层次，和 QEMU 的参数解析搅合在一起，目前我没怎么看 |

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


[^1]: https://virtio-fs.gitlab.io/howto-qemu.html
[^2]: https://en.wikipedia.org/wiki/Pluggable_authentication_module
[^3]: 通过数其源码中 ./target/ 下的目录数确定的
[^4]: https://github.com/firecracker-microvm/firecracker/issues/776
[^5]: https://qemu-project.gitlab.io/qemu/devel/tracing.html
