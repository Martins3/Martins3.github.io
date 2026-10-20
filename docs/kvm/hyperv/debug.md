# 为什么我的 windows 每隔几个小时就会卡死?

## 1

更倾向 KVM，优先怀疑 APIC/IPI 中断投递，但还不能定案。

依据是：
- CPU 0 卡在 KiIpiWaitForRequestBarrier，等待跨 CPU 请求完成。
- CPU 1、2 卡在 KxSetTimeStampBusy，等待同一个 KiTbFlushTimeStamp，说明 TLB 刷新同步没有完成。
- host 侧读取、同步 vCPU 寄存器后，虚拟机随即恢复；Windows 没有重启，SSH、RDP 都恢复正常。

这更像某个 vCPU 没有及时收到或处理中断。Windows 的等待位置已经确认，底层为什么没有完成仍未确认；
缺少卡住时的 LAPIC 状态和 IPI 跟踪，暂时不能排除 Windows 自身的同步 bug。

### 1.1


是通过 QEMU 读到的 RIP，配合 Windows 内核符号和反汇编确定的。

1. 第一次执行 info registers -a 时，CPU 0 输出：

   RIP=fffff807d142f4bc
   RDI=fffff8075d96b180

2. 我把 guest 内存中的内核指令与它的 ntoskrnl.exe 对照，确定运行时内核基址为 fffff807d1200000。匹配的微软 PDB 给出 KiIpiWaitForRequestBarrier 的 RVA 为
   22f490，所以：

   函数地址 = fffff807d1200000 + 22f490
            = fffff807d142f490

   RIP - 函数地址 = 2c

   因而 CPU 0 的执行位置就是：

   nt!KiIpiWaitForRequestBarrier+0x2c

3. 这个位置的实际指令是下面的等待循环：

   fffff807d142f4ba: pause
   fffff807d142f4bc: mov  eax, DWORD PTR [rdi+0x2d80]  ; 当时的 RIP
   fffff807d142f4c2: test eax, eax
   fffff807d142f4c4: jne  fffff807d142f4b0             ; 非零则继续循环

   也就是不断检查一个状态字段，等待它变成零。

严格说，我捕获到的是“CPU 0 当时正在这个等待循环里”。读取寄存器后虚拟机恢复了，因此没有多次采到相同 RIP。结合此前长时间没有磁盘活动、CPU 忙转，以及 CPU
1、2 同时等待 TLB 刷新锁，才判断这是停滞现场；函数位置可以确定，持续卡住的具体原因仍是推断。

## 2

Linux 一直正常，确实是需要考虑的证据。我前面“更倾向 KVM”说得偏强，应该表述为排查优先级，尚不是归因结论。

同一个 KVM，Linux 和 Windows 也可能走不同的路径。Linux 支持 KVM 原生的半虚拟化 TLB 刷新和 IPI；这台 Windows 使用了 hv_vapic 等 Hyper-V
接口，但启动参数没有启用 hv-tlbflush、hv-ipi。因此，仅 Windows 触发的 KVM 问题和 Windows 自身的问题，目前都成立。Linux 稳定不能直接排除前者。内核文档
(https://docs.kernel.org/virt/kvm/x86/cpuid.html)、QEMU 文档 (https://www.qemu.org/docs/master/system/i386/hyperv.html)

我这次是在检查时意外让它恢复的，实际通过 QMP 执行了：

{"execute":"human-monitor-command","arguments":{"command-line":"info registers -a"}}

这条“读寄存器”命令实际会：

1. 把各个 vCPU 从 KVM 执行中踢出来。
2. 将寄存器状态同步到 QEMU。
3. 在重新进入 KVM 前写回运行状态。

这是本机状态同步代码 (/home/martins3/data/qemu/accel/kvm/kvm-all.c:3201)确认的行为，会扰动 vCPU 的执行时序和状态，并非完全被动的观察。Windows 在这次操
作前后恢复，启动时间保持不变。

它可能让未完成的中断处理继续推进，也可能打破了 Windows 内部的某个时序竞争。所以，“读寄存器后恢复”是很有价值的线索，但还不足以证明一定是 KVM 的 bug。


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
