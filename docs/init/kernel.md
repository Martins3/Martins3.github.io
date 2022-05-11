- [ ] 可以将 Loongson 的启动整理过来吗 ?
- [ ] 将陈华才的整理过来一下

## 多核启动[^1]
QEMU 中 cpu_is_bsp 和 do_cpu_sipi

在 x86-64 多核系统的初始化中，需要有一个核作为 bootstrap processor (BSP)，由这个 BSP 来执行操作系统初始化代码。

下面就是加冕仪式了，新晋的 BSP 需要带上王冠（设置自己 IA32_APIC_BASE 寄存器的 BSP 标志位为 1）来表明自己的身份。

那其他的 APs 可不可以也这样做呢，当然不行，否则岂不是要谋反么。此时 APs 进入 wait for SIPI 的状态，等待着 BSP 的发号施令。


[^1]: https://zhuanlan.zhihu.com/p/67989330
