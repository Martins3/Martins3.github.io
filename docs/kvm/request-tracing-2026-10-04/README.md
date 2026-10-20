# KVM request 机制与动态跟踪调查

2026 年 10 月 4 日完成。调查目标是解释 KVM 为什么使用
request、完整执行流程、主要使用者，并在不修改内核源码、不重新加载 KVM
模块的条件下统计请求分布。背景是
[之前通过修改源码统计 request 的记录](../make_request.md)。

核心结论：request 是每个 vCPU 的待办位图，既用于其他线程通知目标
vCPU，也用于当前 vCPU
把工作推迟到合适的执行位置。重复提交可以合并；提交次数、成功检查次数、IPI 次数和
CPU 开销是不同指标。

本机 7 个运行中的 VM，在完整挂载探针后的统一 60 秒窗口内，共观测到 **1,926,891
次提交，约 32,115 次每秒**。steal-time 更新占约一半，IOAPIC 扫描和 EOI bitmap
更新也很活跃。没有人为制造空闲状态。

| 请求             | 原始 req 十六进制 | 提交次数 |   占比 | 普通提交 | 批量路径按目标 vCPU 计数 | 成功检查清位 |
| ---------------- | ----------------: | -------: | -----: | -------: | -----------------------: | -----------: |
| STEAL_UPDATE     |              0x10 |  953,803 | 49.50% |  953,803 |                        0 |      953,197 |
| SCAN_IOAPIC      |             0x317 |  435,274 | 22.59% |  108,823 |                  326,451 |      326,569 |
| LOAD_EOI_EXITMAP |              0x1f |  326,569 | 16.95% |  326,569 |                        0 |      326,569 |
| EVENT            |               0xe |  132,346 |  6.87% |  132,346 |                        0 |      132,346 |
| UNBLOCK          |               0x2 |   52,623 |  2.73% |   52,623 |                        0 |            1 |
| TLB_FLUSH        |             0x300 |   14,258 |  0.74% |   12,018 |                    2,240 |       12,109 |
| MIGRATE_TIMER    |               0x8 |   12,018 |  0.62% |   12,018 |                        0 |       11,864 |

数据来自 [最终原始计数及调用栈](data/counts.jsonl)，汇总见
[summary.json](data/summary.json)。UNBLOCK
探针存在少量漏记，下文说明其边界；这里的数值是观测计数。

**为什么使用 request**

某些操作必须由目标 vCPU 在自己的执行上下文中处理，例如更新 VMCS、同步 MMU
状态和准备事件注入。发起者先更新相关状态，再置位 request，由目标 vCPU
完成后续工作。

发起位置也可能不适合立即完成全部工作。例如 `kvm_arch_vcpu_load()` 会在 vCPU
加载和调度器 preempt notifier 的 sched-in 路径运行；这里设置
`STEAL_UPDATE`，随后在进入 guest 前调用
`record_steal_time()`，把发现需要更新和实际更新 guest 可见状态分开。

位图还可以合并重复工作。相同请求在处理前提交多次，仍然只保留一个
bit。参数和实际事件状态保存在其他结构中，request
表示需要重新检查或同步它们，因此它不是保存每次调用的任务队列。

**执行流程**

```text
发起者更新相关状态
  → kvm_make_request(req, vcpu)
      smp_wmb()
      set_bit(req & KVM_REQUEST_MASK, &vcpu->requests)
  → 必要时 kick 目标 vCPU
      睡眠状态：尝试唤醒
      guest 执行中：根据状态发送 IPI 促使退出
      已在合适的 host 执行路径：可能无需 IPI

QEMU ioctl(KVM_RUN)
  → kvm_arch_vcpu_ioctl_run()
    → vcpu_run()
      → vcpu_enter_guest()
        → kvm_check_request(req, vcpu)
            检查 bit → 清位 → 内存屏障
        → 执行对应处理函数
        → 准备 VM-entry
        → 再次检查 requests 和退出条件
        → 满足条件后进入 guest
```

`kvm_make_request()` 本身不负责唤醒或发送
IPI；相关调用者或批量接口负责通知。多数维护工作在 KVM_RUN
的内核循环中完成，处理后继续运行 guest。多个请求可以共用一次退出 guest；当前
vCPU 给自己提交请求通常无需 IPI。

进入 guest 前的最终检查与 `vcpu->mode`、内存屏障配合，避免请求恰好到达检查与
VM-entry 之间却没有被处理的竞态。处理顺序由代码决定，例如先处理
SCAN_IOAPIC，再处理 LOAD_EOI_EXITMAP，而不是按提交时间排队。

原始 req 的低 8 位是 bit 编号，高位携带控制标志。`0x300` 设置 bit 0，`0x10` 设置
bit 16；高位标志不存入 `vcpu->requests`。

| 标志      | 在相应发送路径中的语义                    |
| --------- | ----------------------------------------- |
| NO_WAKEUP | 不必为了这项维护工作唤醒睡眠 vCPU         |
| WAIT      | 等待所需的 IPI 应答或目标退出相关执行状态 |
| NO_ACTION | 只进行执行状态同步，不设置待处理 bit      |

**WAIT 不等于等待 request 的处理函数执行完。** 对远端 TLB
flush，目标先停止使用旧映射，并保证下次进入 guest
前刷新，就能满足相关同步要求；睡眠 vCPU 可以稍后处理。单独调用
`kvm_make_request()` 也不会因为 req 带 WAIT 就发生等待。

**实际热点及其来源**

STEAL_UPDATE 的 953,803 次提交全部来自 `kvm_arch_vcpu_load()`。该函数也被
`virt/kvm/kvm_main.c` 中的 `kvm_sched_in()`
调用。高提交频次反映经常请求更新，不代表实际 steal time
很大；`record_steal_time()` 还会检查 guest 是否启用相关功能。

本次普通 TLB_FLUSH 的 12,018 次全部来自 `vmx_vcpu_load_vmcs()`，MIGRATE_TIMER 的
12,018 次全部来自 `kvm_arch_vcpu_load()`，两者完全一致。vCPU 转到另一颗物理 CPU
后，VMX 请求刷新 EPT/VPID 相关上下文，通用 x86 路径请求迁移
timer。这为旧记录中两者均占 5.73%
提供了同类机制的实测解释；旧记录本身没有调用栈，不能据此补称当时已经完成同样验证。

此外有 70 次远端 TLB flush 广播，产生 2,240 次目标 vCPU 置位，平均每次 32
个目标。采集的调用栈为：

```text
shmem_fallocate
  → unmap_mapping_pages / 映射失效
    → kvm_mmu_notifier_invalidate_range_start
      → kvm_flush_remote_tlbs
        → kvm_make_all_cpus_request
          → kvm_make_vcpu_request
            → __kvm_make_request(TLB_FLUSH)
```

因此本次 TLB_FLUSH 总量为 12,018 + 2,240 = 14,258。原先只修改
`kvm_make_request()` 的补丁会漏掉批量路径直接调用 `__kvm_make_request()`
的部分。

SCAN_IOAPIC 有两个实测来源：

- guest 写 IOAPIC 寄存器，经 `ioapic_write_indirect()` →
  `kvm_make_vcpus_request_mask()`，产生 326,451 次目标 vCPU 置位。
- `kvm_ioapic_send_eoi()` 直接提交 108,823 次。对应源码中处理旧路由遗留 pending
  EOI 后重新扫描的分支。

消费端形成以下链路：

```text
SCAN_IOAPIC
  → vcpu_scan_ioapic()
  → 更新 ioapic_handled_vectors
  → 提交 LOAD_EOI_EXITMAP
  → vcpu_load_eoi_exitmap()
  → VMX 后端按 APICv 状态更新 VMCS 的 EOI_EXIT_BITMAP
```

SCAN_IOAPIC 检查命中、LOAD_EOI_EXITMAP 提交、LOAD_EOI_EXITMAP 检查命中均为
326,569 次。所有采到的 EOI bitmap 提交栈都来自
`vcpu_scan_ioapic()`。它让硬件知道哪些中断向量的 EOI 需要 KVM
介入；这里的计数不是 EOI VM-exit 次数，也不能保证每次请求最后都进行了 VMCS
写入，因为处理函数有条件判断。

UNBLOCK 的 52,623 次命中全部来自
`apic_timer_expired()`。`kvm_check_request(UNBLOCK)` 虽然只命中一次，但
`vcpu_run()` 还有直接 `kvm_clear_request(UNBLOCK)`
的路径，因此不能把其余提交解释成丢失。EVENT 的 132,346 次提交全部来自
`apic_set_eoi()`，本次检查端计数同为 132,346。

| 机制类别           | 代表请求                                                              | 后续工作                           |
| ------------------ | --------------------------------------------------------------------- | ---------------------------------- |
| 内存虚拟化         | TLB_FLUSH、TLB_FLUSH_CURRENT、TLB_FLUSH_GUEST、MMU_SYNC、LOAD_MMU_PGD | 刷新转换，同步或切换 MMU 状态      |
| 中断与事件         | EVENT、NMI、SMI、UNBLOCK                                              | 事件检查、准备注入、解除阻塞       |
| 中断控制器同步     | SCAN_IOAPIC、LOAD_EOI_EXITMAP、APICV_UPDATE                           | 同步路由、拦截位图及 APICv 状态    |
| 时间与调度         | STEAL_UPDATE、MIGRATE_TIMER、CLOCK_UPDATE                             | 更新时间信息及 timer 所在 CPU      |
| 性能监控           | PMU、PMI                                                              | 重编程计数器、处理性能中断         |
| 生命周期和其他控制 | VM_DEAD、DIRTY_RING_SOFT_FULL、APF 等                                 | 终止运行、通知用户态、处理异步缺页 |

**探测方法与验证边界**

运行内核为 `7.2.7-200.fc44.x86_64`，不是工作目录 linux-drm 当时构建的
`7.2.0-00001-geb5a10dc0e00`。使用运行内核的匹配 debuginfo，核对加载模块的
build-id：

```text
kvm        bd7517a3f50e7f88ad10d00890c65ca6abf9a3d5
kvm_intel  f33294539be3205c9bf026e29e643700e94ba5c4
```

环境信息见 [run.json](data/run.json)。其中 start_time
是采样器启动时间，包含准备阶段，不应当作精确的 60 秒窗口起点；BPF 使用
`window_start_ns` 标记实际统一窗口。

`perf probe` 能找到内联位置，但本机 perf 不能处理 req 使用的一种 DWARF
表达式。因此使用 pyelftools 读取常量与内联范围，再用 objdump
对照真实指令，生成偏移 kprobe：151 个普通提交点、1 个批量提交点、48
个成功检查清位点，加上辅助探针，总计 208 个探针。

批量提交点的 req 从经 DWARF 和反汇编核对的 RSI
读取。编译器合并的两条事件路径共用已有置位指令，计数时去重；见
[shared-sites.json](data/shared-sites.json)。DWARF
枚举范围内没有剩余未解析实例，见 [inventory.json](data/inventory.json) 与
[unresolved.json](data/unresolved.json)。这不是对任意内核版本的覆盖保证。

两处 `kvm_ioapic_send_eoi.part.0` 探针在 bpftrace
的按符号枚举中不可用，改用核对后的实时绝对地址。因此需要 bpftrace 的 `--unsafe`
选项；程序只读和计数，不修改内核业务数据。

前两轮全量试验出现提交端与检查端计数不一致，不能作为最终分布。小规模 10
秒验证中，IOAPIC 扫描入口、SCAN_IOAPIC 清位、EOI bitmap 提交和清位四者均为
55,789。完整采样最终通过 `bpftrace -c` 启动子进程，在探针挂载完成后由其 getpid
系统调用设置统一时间起点，所有探针使用相同的 60
秒时间谓词。定时器只负责窗口结束后的退出。

仅在 BEGIN 或第一个 interval 中开始计时不够稳妥：BEGIN
发生在其他探针挂载之前，大批探针逐个挂载期间也可能已经触发 interval。最终复测中
IOAPIC 链路三处计数一致，迁移相关两处计数一致，提交汇总等于普通与批量计数之和。

采样快照中没有观测到 BPF recursion
misses。整个探针挂载期间，`apic_timer_expired+116` 和辅助 `__kvm_vcpu_kick`
探针分别观测到最多 56、52 次 kprobe
missed。它们包含窗口之外的时间，不能直接加回表格；UNBLOCK
计数不能宣称完全无损。其他已采集链接的 missed
为零。探针自身也会影响调度和执行时间，本次频次不是 CPU 开销测量。

最终程序退出码为 0，没有新增动态事件残留；见
[validation.json](data/validation.json) 和 [cleanup.json](data/cleanup.json)。

**保存的材料与重启后使用方式**

- `data/`：最终计数、汇总、环境、覆盖清单和验证结果，可直接阅读。
- `scripts/`：Python 脚本审阅副本，其中 prepare.py 补充了 ELF
  类型检查及类型标注；它们原本共处一个工作目录，不应直接在此子目录运行。
- `artifacts.tar.gz`：原临时目录的完整文件归档，不含 Python
  缓存。包含脚本、解压后的精确 KVM 模块、最终和废弃轮次的计数、BPF
  程序、调试快照及日志。
- `build-reference.tar.gz`：运行内核对应的 KVM debuginfo
  和主要源码文件，供离线复查。
- `SHA256SUMS`：持久化文件的校验和。

归档中的 `counts.jsonl` 是最终有效结果；`counts.round1.jsonl` 和
`counts.round2.jsonl` 是已弃用的早期试验，仅供排查过程追溯。

重启后复跑时先解压到新的工作目录，避免覆盖此次保存的证据：

```bash
mkdir -p ~/data/kvm-request-replay
tar -xzf /home/martins3/data/vn/docs/kvm/request-tracing-2026-10-04/artifacts.tar.gz -C ~/data/kvm-request-replay
sudo /usr/bin/python3 ~/data/kvm-request-replay/collect.py
/usr/bin/python3 ~/data/kvm-request-replay/summarize.py
```

必须通过 `collect.py` 重新解析符号地址，**不要直接执行归档中的
requests.bt**：其中包含上次启动的绝对地址，重启后可能失效。采样器会检查运行内核版本、KVM
模块 build-id 和符号，并重新生成
requests.bt。如果内核或模块构建变了，需要重新解析 debuginfo、核对指令并生成
inventory，不能照搬偏移。

采样脚本依赖本机 `/home/martins3/.nix-profile/bin/bpftrace` 和 `bpftool`；离线
prepare 还依赖 `/usr/bin/python3` 的 pyelftools、objdump，以及它所引用的匹配
debuginfo 路径。脚本针对本次构建编写，尚不是跨版本通用工具。文件中没有保存 sudo
密码。

源码按函数定位，不依赖易变化的行号：

| 源码文件                 | 关键函数或定义                                                                                             |
| ------------------------ | ---------------------------------------------------------------------------------------------------------- |
| include/linux/kvm_host.h | KVM_REQUEST 标志、kvm_make_request、__kvm_make_request、kvm_check_request、kvm_vcpu_exiting_guest_mode     |
| virt/kvm/kvm_main.c      | kvm_make_vcpu_request、kvm_make_all_cpus_request、kvm_flush_remote_tlbs、__kvm_vcpu_kick、kvm_sched_in     |
| arch/x86/kvm/x86.c       | kvm_arch_vcpu_load、record_steal_time、vcpu_enter_guest、vcpu_run、vcpu_scan_ioapic、vcpu_load_eoi_exitmap |
| arch/x86/kvm/vmx/vmx.c   | vmx_vcpu_load_vmcs、vmx_flush_tlb_all、vmx_load_eoi_exitmap                                                |
| arch/x86/kvm/lapic.c     | apic_timer_expired、apic_set_eoi、kvm_ioapic_send_eoi                                                      |
| arch/x86/kvm/ioapic.c    | ioapic_write_indirect、kvm_ioapic_scan_entry                                                               |
| arch/x86/kvm/regs.h      | leave_guest_mode 中延后的 LOAD_EOI_EXITMAP 提交                                                            |

本机原始源码根目录为
`/usr/src/debug/kernel-7.2.7/linux-7.2.7-200.fc44.x86_64/`。机制参考
[内核 VCPU Requests 文档](https://docs.kernel.org/virt/kvm/vcpu-requests.html)，探针语义参考
[bpftrace 0.25 文档](https://bpftrace.org/docs/release_025/language)。

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
