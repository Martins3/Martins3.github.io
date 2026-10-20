## opcode_table 的使用位置

```c
static const struct opcode opcode_table[256];

static const struct opcode twobyte_table[256];
```

```c
struct opcode {
    u64 flags : 56;
    u64 intercept : 8;
    union {
        int (*execute)(struct x86_emulate_ctxt *ctxt);
        const struct opcode *group;
        const struct group_dual *gdual;
        const struct gprefix *gprefix;
        const struct escape *esc;
        const struct instr_dual *idual;
        const struct mode_dual *mdual;
        void (*fastop)(struct fastop *fake);
    } u;
    int (*check_perm)(struct x86_emulate_ctxt *ctxt);
};
```


## emulate_ops 只有一个调用，为什么做成回调?
就当前代码而言，回调不是功能上的必需，改成直接函数调用也是可行的。 这张表的价值主要在于表达模拟器与执行环境之间的接口。

例如 arch/x86/kvm/emulate.c:1320：

rc = ctxt->ops->read_emulated(ctxt, addr, buffer, size,
                             &ctxt->exception);

模拟器负责解码指令、计算地址、处理操作数；这个接口负责“从 guest 地址读取数据”。地址翻译、跨页访问、MMIO、退出用户态等细节由 x86.c
处理。把这些操作集中在 ops 中，可以明确看到模拟器需要执行环境提供哪些能力。

不过，分层本身并不要求函数指针。 普通函数接口同样可以保留这个边界。当前只有一个实现，使用 ops 确实增加了一层间接调用，收益需要结合
历史理解：文件开头 (arch/x86/kvm/emulate.c:1) 明确标注模拟器源自 Xen，并将它描述为通用 x86 指令解码器和模拟器，这种接口形式与可复用
组件的设计相符。

因此，更准确的说法是：这是保留下来的接口抽象；当前树没有利用它切换实现。 而且现在 emulate.c 已经包含 KVM 专用头文件，也不能仅凭这张
ops 表就认为模拟器完全独立于 KVM。

## 为什么需要那么多状态和回调

### 5.1 x86 指令语义决定了模拟器的规模

完整模拟某一条指令，可能涉及以下任意组合：

- **编码与模式**：16/32/64 位、前缀、ModRM/SIB、位移、立即数、操作数/地址宽度。
- **地址与权限**：段基址、段界限、canonical address、guest 页表、CPL、 TSS I/O
  bitmap；错误时应产生 #GP、#SS、#PF、#UD 等正确异常。
- **隐含状态**：栈、RFLAGS、字符串指令的 RSI/RDI/RCX/DF， STI/MOV SS 的
  interrupt shadow、单步 #DB。
- **执行进度**：REP 可以只完成一部分；MMIO 读可能要等 QEMU；
  一条指令的内存访问可能跨页。
- **同步与原子访问**：软件执行需要和其他 vCPU 的内存访问协调。

`emulate.c` 的 `opcode_table`、`twobyte_table` 和各类 group/prefix
表不只描述长度： `struct opcode` 还指定操作数规则、执行方法、权限检查和 nested
intercept 类型。 解码器把这些规则与当前 guest
模式组合，得到后续执行需要的上下文。

例如，模拟 INS 不只是读取一个端口：还需要检查 I/O 权限，并把结果写入 guest
内存。 `emulate.c` 的 `emulator_io_port_access_allowed()` 会读取 TSS 中的 I/O
bitmap， 这就是为什么模拟 I/O 仍然需要普通内存读取和段状态。

### 2.4 为什么调用链这么深

以 MMIO 写为例，把 `emulate.c` 与 `x86.c` 两层展开后就是：

```text
x86_emulate_instruction()                  # KVM 总控，x86.c
  -> x86_decode_emulated_instruction()
     -> init_emulate_ctxt()                # guest 模式、RIP、flags、缓存
     -> x86_decode_insn()                  # emulate.c，解码与操作数定位
  -> x86_emulate_insn()                    # emulate.c，执行指令语义
     -> em_mov()
     -> writeback()
        -> segmented_write()
           -> linearize()                  # 分段与地址检查
           -> emulate_ops.write_emulated()
              -> emulator_write_emulated() # 回到 x86.c
                 -> emulator_read_write()
                    -> emulator_read_write_onepage()
                       -> 尝试 guest RAM
                       -> 尝试内核 MMIO 处理
                       -> 未处理部分记为 MMIO fragment
  -> 写回 flags、RIP、更新中断状态等
  -> 必要时返回用户态，exit_reason = KVM_EXIT_MMIO
```

这些层次分别负责指令语义、guest 地址语义、内存/设备访问和 KVM 运行状态。
深调用链并不意味着每层都在模拟一个独立设备，也不是再次运行整个 guest。

`emulator_read_write_onepage()` 先尝试 guest RAM，再尝试内核设备处理；
一条指令可以同时访问普通内存与 MMIO，跨页时还可能得到不连续的 GPA。
因此不能把所有操作数访问都简单转发为一个 QEMU 请求。


## 那就是为什么 arm 没有那么复杂? 

差别不只是指令定长
`arch/arm64/kvm/mmio.c` 的 `io_mem_abort()` 在 syndrome 有效时， 直接从 ESR
获取读写方向、访问大小、Rt，以及符号扩展/寄存器宽度相关信息。
设备读完成后，`kvm_handle_mmio_return()` 据此写回寄存器并推进 PC。

这种 load/store 指令模型与硬件提供的信息，使普通 MMIO 不需要 x86 这样广泛的
软件解码和指令执行机制。关键是**指令语义和 trap 信息是否足够**， 不只是“ARM
指令长度固定”。

ARM64 也有 syndrome 无效或复杂访存的情况。当前代码会视配置返回
`KVM_EXIT_ARM_NISV`、`KVM_EXIT_ARM_LDST64B`，或报错/向 guest 注入异常，
并不是所有指令都能靠 ESR 自动完成。


复杂性来自 16/32/64 位模式、分段、权限与异常、flags、原子操作、REP 部分完成，以及跨用户态继续执行。变长指令只是其中一部分。


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
