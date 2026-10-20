## 4. 除了 MMIO，还有哪些场景

### 4.1 shadow paging 为何需要普通内存写模拟

guest 页表本身也是普通 RAM。guest 用 MOV、CMPXCHG 等普通指令写 PTE 时， KVM
为了维护 shadow 页表，可能将对应页写保护并截获写访问。

此时需要在保持跟踪的前提下完成这一笔写入。`x86.c` 中的 `emulator_write_guest()`
写 guest RAM 后调用 `kvm_page_track_write()`， 供相应的页表同步/跟踪逻辑处理。

这与 dirty logging 的写保护要区分：dirty logging 通常标脏、恢复可写映射后重试，
不需要把每次写都交给指令模拟器。普通 EPT guest 也不必为了每次 guest PTE
更新退出； shadow paging、nested shadow EPT/NPT 等场景才更相关。

KVM 也会尝试解除 shadow 写保护并重试，避免不必要的模拟；但如果解除保护同时销毁了
该指令依赖的翻译，就可能形成“建表、写保护、退出、拆表”的循环。
`EMULTYPE_WRITE_PF_TO_SP` 用于标识这种不能在模拟失败后简单拆表重试的情况。

参见本地 `Documentation/virt/kvm/x86/mmu.rst` 的 “Synchronized and
unsynchronized pages”“Reaction to events”，以及
[上游 KVM MMU 文档](https://docs.kernel.org/virt/kvm/x86/mmu.html#synchronized-and-unsynchronized-pages)。

### 4.2 vmx_emulation_required() 并非永远返回 0

当前源码中：

```c
bool vmx_emulation_required(struct kvm_vcpu *vcpu)
{
    return emulate_invalid_guest_state && !vmx_guest_state_valid(vcpu);
}
```

`vmx/vmx.h` 中的 `vmx_guest_state_valid()` 在 unrestricted guest 可用时直接返回
true。 这解释了为什么在现代配置下经常观察到“不需要该回退”。

这是特定运行配置的结果。`emulate_invalid_guest_state` 当前仍默认开启；
在没有启用 unrestricted guest、段寄存器状态不满足 VMX 检查等情况下，
`handle_invalid_guest_state()` 仍会循环调用模拟器。 不能把它理解成“所有 VM
启动时都必须跑一段软件模拟”。

启动期的固件与 OS 设备初始化也会访问 MMIO 或使用 string I/O，所以启动时观察到
模拟器调用，不能直接归因于 real-mode 回退。运行期同样可能因设备访问进入模拟器；
具体次数取决于设备模型、驱动和硬件加速配置。

### 4.3 #UD 不表示 KVM 能兜底任意新指令

`handle_ud()` 默认传入 `EMULTYPE_TRAP_UD`； `x86_decode_insn()` 只允许带
`EmulateOnUD` 标记的指令进入这一兼容路径。 当前表中包括某些
hypercall、MOVBE、RDPID、SYSCALL/SYSENTER/SYSEXIT、RSM 等条目，
每条指令仍有自己的执行模式、feature 和权限条件。

普通非法指令仍应向 guest 注入 #UD。强制模拟测试使用 `EMULTYPE_TRAP_UD_FORCED`
和显式开启的 force-emulation prefix，是另一条路径。

### 4.4 嵌套虚拟化增加条件，但不是模拟器存在的前提

非嵌套 VM 的 MMIO 和 string I/O 已经足够需要模拟器。 模拟 L2 指令时，还需要考虑
L1 的拦截条件、异常优先级和地址翻译。

`x86_emulate_instruction()` 首次模拟 L2 时传入 `check_intercepts`； I/O
completion 的 `EMULTYPE_NO_DECODE` 路径不重复检查已经处理过的拦截。

此外，部分复杂操作直接复用模拟器组件，不经过完整的指令入口：

- `x86.c` 的 `kvm_task_switch()` 调用 `emulator_task_switch()`。
- `kvm_inject_realmode_interrupt()` 调用 `emulate_int_real()`。
- SMM 恢复使用 `smm.c` 中的 `emulator_leave_smm()`。

这些操作也解释了为什么模拟器需要处理 TSS、段、IDT/GDT 等状态。

### 5.2 emulate_ops 与 kvm_x86_ops 的边界

| 接口                              | 服务对象       | 作用                                               |
| --------------------------------- | -------------- | -------------------------------------------------- |
| `x86_emulate_ops` / `emulate_ops` | 通用指令模拟器 | 提供访问 guest 内存、寄存器、设备和 CPU 状态的方法 |
| `kvm_x86_ops`                     | KVM x86 公共层 | 连接 VMX/SVM 等后端，操作硬件虚拟化状态            |

`emulate.c` 描述“指令应当做什么”；`x86.c` 的 `emulate_ops` 实现 “如何在 KVM
中访问这个 guest”；VMX/SVM 后端再处理各自的 VMCS/VMCB。

例如，`emulator_get_idt()` 最终使用 `kvm_x86_call(get_idt)` 取得 guest IDT。
一次指令已经退出到软件执行后，模拟器需要主动读取这些状态，不能再依赖 CPU
继续执行该指令时产生其他 VM exit。

内存回调也有不同职责，见 `kvm_emulate.h` 的 `struct x86_emulate_ops`：

- `fetch`：从普通 guest 内存取指。
- `read_std/write_std`：普通内存访问，主要用于描述符等隐含访问。
- `read_emulated/write_emulated`：指令操作数访问，可能是 RAM，也可能是 MMIO，
  需要设备路由和用户态 completion。
- `cmpxchg_emulated`：供 LOCK 等操作使用的 compare-exchange 路径。

### 5.3 RIP 更新与原子性都不是一个简单收尾动作

`x86_decode_insn()` 解码时推进内部 `_eip`，但不等于已经提交 guest RIP。
正常完成后，`x86_emulate_insn()` 更新 `ctxt->eip`，总控再写回 guest。
fault、等待 MMIO 读、REP 尚未结束等情况下，RIP 的处理不同； 分支指令更不能按“原
RIP + 指令长度”处理。

`EMULTYPE_SKIP` 则只借用解码器找下一 RIP，不执行指令语义。 VMX
通常已有长度，只有某些信息缺失场景需要这一回退； 例如运行在其他 hypervisor
上时，EPT misconfig 的指令长度并不保证有效。

原子性也不能靠“该 vCPU 暂停了”保证。对于能直接访问的、符合大小和边界条件的 guest
RAM，`emulator_cmpxchg_emulated()` 使用 host 原子 compare-exchange； `emulate.c`
的 `writeback()` 在 LOCK 路径调用这一回调。 但
MMIO、跨边界等情况存在降级为普通写的分支，不能把它当成对任意地址提供完整
原子事务保证的机制。

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
