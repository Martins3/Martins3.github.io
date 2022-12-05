# 如何设计一个成功的指令集架构

显然我的功力不足以真正的指导一个指令集架构的设计，感觉 10 年前是 X86 一家独大，但是如今指令集又有一种百家争鸣，工作学习中时不时和不同的指令集打交道，用这篇 blog 来整理一下自己在学习不同架构中的内容。
同时欢迎大家的批评指正。

实现一个简单的 CPU 是不难的，如果仅仅包含运算指令，跳转指令，访存指令，其强度大约是本科生的大作业，但是能够跑起来 Linux 操作系统，就开始有点麻烦了，不过还好。
但是如果你的 CPU 可以:
- 在嵌入式平台中运行，这要求超低功耗。
- 可以在数据中心中运行，这要求虚拟化，高性能，多核。
- 可以在桌面电脑中运行，这要求视频编解码，软件生态。

这些要求让躺在 linux/arch 和 QEMU/target 下架构，除了 X86，ARM 和 RISC，大多死得悄无声息。


## 软件支持

### 基础库
- LibC

### 硬件虚拟化加速

能够支持 hyerpv, kvm, zen 等，也就是至少需要考虑:
- 如何实现 CPU 虚拟化和内存虚拟化

参考 Intel 的 VT-x

### 编译器
需要以下的编译器的支持:
- LLVM / GCC
- JVM , LuaJit 和 .NET
- Rust / Golang

需要有:
- abi 定义
- elf 支持

### 高性能计算

### QEMU
主要是 tcg 和 machine 的定义。

### Linux 内核
让 MacOS 或者 Windows 这种大公司持有的闭源操作系统支持你的新架构，可能性实在是太小了。

- 内存: page walk, cache，TLB
- ebpf
- 中断，异常
- KVM
- idle driver
- init
- perf
- vdso
- memory model
- memory management
  - page table
  - ioremap
  - hugetlb
- ACPI
- 锁
- trace
  - krpoeb / uprobe
  - pmc : 要求 CPU 有硬件计数器
- kdump
- mmio
- kgdb
- context switch
  - `__switch_to`
- fork
  - `arch_dup_task_struct`
  - `copy_thread`
  - `ret_from_fork`
- boot
  - `setup_arch`
- signal
- 各种使用汇编写的库

### Firmware
#### UEFI

已经存在开源的 edk2 ，但是需要架构的支持。

#### ACPI

将开源的 acpica 支持上。

## 硬件设计
- 指令是否定长，不定长会让译码很难做，但是对齐之后，加载一个 8 字节的指令需要多条指令。
- 如何避免指令的相关性。
- 那些是当前负载中经常出现的指令。
- 指令的复杂程度不是功耗的决定因素[^2]。
- 最精简的指令集只是需要一条[^3] [^4]。

- 在用户态执行的指令集和系统态执行的指令集不同。
- 不同的核执行的指令集不同[^5]。

### IOMMU
IOMMU 可以实现 irq remapping 和 dma remapping 进而:
- 安全上可以保护硬件；
- 实现设备直通给 Guest 虚拟机；
- Linux 利用 IOMMU 实现 VFIO 进而支持 SPDK 和 SPDK 等 kernel bypass 驱动。

具体可以参考 Intel 的 VT-d

### 错误检测
例如 ECC，需要考虑实现 edac

### SOC
- 主要要求实现 chipset

参考 [Intel PCH](https://en.wikipedia.org/wiki/Platform_Controller_Hub)

### memory model
懂的都懂，很关键。

### cache coherence
- 如果 dma 修改了 memory ，如何同步到 cache 中去

### NUMA
NUMA 总线协议，更多的总线，更加复杂的 irqbalance 要求，需要内核，ACPI 和用户态工具特殊处理 NUMA .

### 总线协议
- 片内使用何种协议
- 和设备沟通的片外协议如何操作

### 外设
- 使用 memory mapped io 还是单独的 io 指令

### 调试
没有硬件支持，类似 gdb 中的 watch 和 breakpoint 的效率非常低。

很多虚拟机中也是支持对于 guest 调试的，也是需要硬件支持的。

参考:
- https://wiki.segger.com/Semihosting

### 硬件性能计数器

参考 [Intel PCM](https://github.com/intel/pcm)

### 物理层设计
没有搞过，不是特别懂，但是是存在这一个步骤的。

### 编译器
- 各种 built-in

### 加速
- SIMD 指令
- 加解密指令

### 二进制翻译
存在特殊的指令来加速二进制翻译

### 操作系统
- swap: 如何确定一个页面是否访问，到底是在 page table entry 上放一个 flags，还是使用 page fault 来实现。

### 兼容性

### 功耗

#### 运行功耗控制
功耗对于嵌入式设备或者移动设备极其重要，如果不小心考虑，相同性能，几十倍的功耗是很容易搞出来的。

https://www.zhihu.com/question/26655435/answer/1825719171

#### 动态功耗调整
- pstate : 高负载的时候如何提升频率；
- cstate : 低负载的时候如何控制功耗；

参考: https://en.wikipedia.org/wiki/Advanced_Configuration_and_Power_Interface

### 可靠性
- ECC 内存

### 成本
应该主要体现在面积上

## TODO
- 测试 RISC-V 的用户态中断，硬件线程

## 其他的思考
- intel 可以设置每一个核使用的 cache 多少
  - https://intel.github.io/cri-resource-manager/stable/docs/policy/rdt.html

## 参考资料

[^1]: [riscv non isa](https://github.com/riscv-non-isa)
[^2]: [Power Struggles: Revisiting the RISC vs. CISC Debate on Contemporary ARM and x86 Architectures](https://research.cs.wisc.edu/vertical/papers/2013/hpca13-isa-power-struggles.pdf)
[^3]: [One-instruction set computer](https://en.wikipedia.org/wiki/One-instruction_set_computer)
[^4]: https://en.wikipedia.org/wiki/No_instruction_set_computing
[^5]: A reconfigurable heterogeneous multicore with a homogeneous ISA


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
