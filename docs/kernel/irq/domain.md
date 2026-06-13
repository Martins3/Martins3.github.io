#### irq domain 的结构
1. 在 arch_early_irq_init 会初始化 16 个 irq_desc

紧接着创建变量
```c
x86_vector_domain = irq_domain_create_tree(fn, &x86_vector_domain_ops, NULL);
```

- [ ] 前 16 是这个时候创建，之后的是怎么创建的?

2. vector 在上，而 mp_ioapic_irqdomain_ops

```c
static const struct irq_domain_ops x86_vector_domain_ops = {
  .select   = x86_vector_select,
  .alloc    = x86_vector_alloc_irqs,
  .free   = x86_vector_free_irqs,
  .activate = x86_vector_activate,
  .deactivate = x86_vector_deactivate,
#ifdef CONFIG_GENERIC_IRQ_DEBUGFS
  .debug_show = x86_vector_debug_show,
#endif
};

const struct irq_domain_ops mp_ioapic_irqdomain_ops = {
  .alloc    = mp_irqdomain_alloc,
  .free   = mp_irqdomain_free,
  .activate = mp_irqdomain_activate,
  .deactivate = mp_irqdomain_deactivate,
};
```
x86_vector_domain_ops 和 mp_ioapic_irqdomain_ops 其实就是对应 lapic 和 ioapic:

- mp_ioapic_irqdomain_ops
  - mp_register_handler : 注册了 `__irq_set_handler`

- [ ] 不知道为什么 mp_ioapic_irqdomain_ops 正好不会注册 2 号?
  - [ ] 虽然 0 号 pin 没有连接上

从 mp_register_handler 到达的：
```txt
#0  x86_vector_alloc_irqs (domain=0xffff88810004d180, virq=1, nr_irqs=1, arg=0xffffffff82403df0) at arch/x86/kernel/apic/vector.c:533
#1  0xffffffff8104a2f1 in mp_irqdomain_alloc (domain=0xffff8881000fe000, virq=1, nr_irqs=1, arg=0xffffffff82403df0) at arch/x86/kernel/apic/io_apic.c:3020
#2  0xffffffff810c1c35 in irq_domain_alloc_irqs_hierarchy (arg=0xffffffff82403df0, nr_irqs=1, irq_base=1, domain=0xffff8881000fe000) at kernel/irq/irqdomain.c:1383
#3  __irq_domain_alloc_irqs (domain=domain@entry=0xffff8881000fe000, irq_base=irq_base@entry=1, nr_irqs=nr_irqs@entry=1, node=node@entry=-1, arg=arg@entry=0xffffffff824 03df0, realloc=realloc@entry=true, affinity=0x0 <fixed_percpu_data>) at kernel/irq/irqdomain.c:1439
#4  0xffffffff8104922a in alloc_isa_irq_from_domain (domain=domain@entry=0xffff8881000fe000, irq=irq@entry=1, ioapic=ioapic@entry=0, info=info@entry=0xffffffff82403df0, pin=1) at arch/x86/kernel/apic/io_apic.c:1008
#5  0xffffffff81049f2a in mp_map_pin_to_irq (gsi=1, idx=<optimized out>, ioapic=<optimized out>, pin=pin@entry=1, flags=1, info=info@entry=0x0 <fixed_percpu_data>) at a rch/x86/kernel/apic/io_apic.c:1057
#6  0xffffffff8104a0c9 in pin_2_irq (idx=<optimized out>, ioapic=ioapic@entry=0, pin=pin@entry=1, flags=flags@entry=1) at arch/x86/kernel/apic/io_apic.c:1103
#7  0xffffffff82b5f326 in setup_IO_APIC_irqs () at arch/x86/kernel/apic/io_apic.c:1219
#8  setup_IO_APIC () at arch/x86/kernel/apic/io_apic.c:2416
#9  0xffffffff82b52ce4 in x86_late_time_init () at arch/x86/kernel/time.c:100
#10 0xffffffff82b4bfcb in start_kernel () at init/main.c:1051
#11 0xffffffff81000107 in secondary_startup_64 () at arch/x86/kernel/head_64.S:283
#12 0x0000000000000000 in ?? ()
```

从 msi_domain_alloc 到达的 :
```txt
#0  x86_vector_alloc_irqs (domain=0xffff88810004d180, virq=24, nr_irqs=1, arg=0xffffc90000043c18) at arch/x86/kernel/apic/vector.c:533
#1  0xffffffff810c3982 in msi_domain_alloc (domain=0xffff88810004df00, virq=24, nr_irqs=1, arg=0xffffc90000043c18) at kernel/irq/msi.c:150
#2  0xffffffff810c1c35 in irq_domain_alloc_irqs_hierarchy (arg=0xffffc90000043c18, nr_irqs=1, irq_base=24, domain=0xffff88810004df00) at kernel/irq/irqdomain.c:1383
#3  __irq_domain_alloc_irqs (domain=domain@entry=0xffff88810004df00, irq_base=irq_base@entry=-1, nr_irqs=1, node=<optimized out>, arg=arg@entry=0xffffc90000043c18, real loc=realloc@entry=false, affinity=0x0 <fixed_percpu_data>) at kernel/irq/irqdomain.c:1439
#4  0xffffffff810c3f88 in __msi_domain_alloc_irqs (domain=0xffff88810004df00, dev=0xffff8881002e08c0, nvec=<optimized out>) at ./include/linux/device.h:636
#5  0xffffffff81448c44 in msix_capability_init (affd=<optimized out>, nvec=<optimized out>, entries=0x0 <fixed_percpu_data>, dev=0xffff8881002e0800) at drivers/pci/msi. c:788
#6  __pci_enable_msix (flags=<optimized out>, affd=<optimized out>, nvec=<optimized out>, entries=0x0 <fixed_percpu_data>, dev=0xffff8881002e0800) at drivers/pci/msi.c: 1003
#7  __pci_enable_msix_range (flags=<optimized out>, affd=<optimized out>, maxvec=<optimized out>, minvec=<optimized out>, entries=<optimized out>, dev=<optimized out>) at drivers/pci/msi.c:1137
#8  __pci_enable_msix_range (dev=0xffff8881002e0800, entries=0x0 <fixed_percpu_data>, minvec=<optimized out>, maxvec=<optimized out>, affd=<optimized out>, flags=<optim ized out>) at drivers/pci/msi.c:1117
#9  0xffffffff81448e80 in pci_alloc_irq_vectors_affinity (dev=dev@entry=0xffff8881002e0800, min_vecs=min_vecs@entry=1, max_vecs=max_vecs@entry=1, flags=flags@entry=7, a ffd=affd@entry=0x0 <fixed_percpu_data>) at drivers/pci/msi.c:1206
#10 0xffffffff816e2b5a in pci_alloc_irq_vectors (flags=7, max_vecs=1, min_vecs=1, dev=0xffff8881002e0800) at ./include/linux/pci.h:1824
#11 nvme_pci_enable (dev=0xffff8881003f4000) at drivers/nvme/host/pci.c:2381
#12 nvme_reset_work (work=0xffff8881003f46d0) at drivers/nvme/host/pci.c:2605
#13 0xffffffff8107f9ef in process_one_work (worker=0xffff88810004d9c0, work=0xffff8881003f46d0) at kernel/workqueue.c:2275
#14 0xffffffff8107fbc5 in worker_thread (__worker=0xffff88810004d9c0) at kernel/workqueue.c:2421
#15 0xffffffff81086109 in kthread (_create=0xffff888100123040) at kernel/kthread.c:313
#16 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
*/
```


## 研究下，hwirq 到 irq 的转换过程是什么

基本案例
```txt
[root@nixos:/sys/kernel/debug/irq/irqs]# cat 91
handler:  handle_edge_irq
device:   0000:08:00.4
status:   0x00004000
istate:   0x00004000
ddepth:   0
wdepth:   0
dstate:   0x35409200
            IRQD_ACTIVATED
            IRQD_IRQ_STARTED
            IRQD_SINGLE_TARGET
            IRQD_MOVE_PCNTXT
            IRQD_AFFINITY_SET
            IRQD_AFFINITY_ON_ACTIVATE
            IRQD_CAN_RESERVE
            IRQD_HANDLE_ENFORCE_IRQCTX
node:     -1
affinity: 11
effectiv: 11
pending:
domain:  IR-PCI-MSIX-0000:08:00.4-12
 hwirq:   0x0                       <---- 就是这个 PCI 设备的第几个中断
 chip:    IR-PCI-MSIX-0000:08:00.4
  flags:   0x430
             IRQCHIP_SKIP_SET_WAKE
             IRQCHIP_ONESHOT_SAFE
 parent:
    domain:  AMD-IR-0-14
     hwirq:   0x8040000  <---- 应该类似
     chip:    AMD-IR
      flags:   0x0
     parent:
        domain:  VECTOR
         hwirq:   0x5b <----- 这个正好就是 91 的
         chip:    APIC
          flags:   0x0
         Vector:    37 <----- CPU 中的 Vector
         Target:    11 <----- 目标 CPU
         move_in_progress: 0
         is_managed:       0
         can_reserve:      1
         has_reserved:     0
         cleanup_pending:  0
```

### /sys/kernel/irq 中的 hwirq 如何理解?

就是最下一层的 irq 了。这是合理的，irq_desc 总是管理到最下一层的，然后逐级向上，然后一直找到 CPU

## irq domain 相关的结构体
<!-- 01ea2fe1-3fa3-4a1d-aaa7-6704aa1c7b2c -->

- struct irq_desc
- struct irq_action
- struct irq_data : 一个 irq_desc 在每一个 domain 中关联一个 irq_data
- struct irq_domain
- struct irq_domain_ops
- struct irqaction
- struct irq_chip

其实这个图搞出来，就很容易理解了:
也就是
1. 一个 virq 会对应一个 irq_desc
2. 一个 irq_desc 内部放一个 irq_common_data 和 irq_data
3. irq_desc 找到 irq_data ，然后通过 irq_data 找到 irq_domain
4. 一个 irq_desc 会对应个 num(irq_data) ，用来记录这个 irq_desc 在每一级 irq_domain 中的内容
5. 一个 domain 中的 irq_data 都指向一个 irq_common_data ，但是 root 的除外，因为一个 irq_desc 本来就包含一个 irq_common_data
为什么这样设计，原因未知

```txt
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           struct irq_desc (中断描述符)                          │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────────────┐  │
│  │ irq_common_data │  │   irq_data      │  │      *action (irqaction链)      │  │
│  │  (共享数据)     │  │ (per-domain数据)│  │                                 │  │
│  └─────────────────┘  └────────┬────────┘  └─────────────────────────────────┘  │
└────────────────────────────────┼────────────────────────────────────────────────┘
                                 │
                                 ▼
                    ┌────────────────────────┐
                    │    struct irq_data     │
                    │  ┌──────────────────┐  │
                    │  │ irq  (虚拟中断号)│  │
                    │  │ hwirq(硬件中断号)│  │
                    │  ├──────────────────┤  │
                    │  │ *common          │──┼──> irq_common_data (共享状态/亲和性)
                    │  │ *chip            │──┼──> struct irq_chip (硬件操作接口)
                    │  │ *domain          │──┼──> struct irq_domain (所属domain)
                    │  │ *parent_data     │──┼──> 层级: 父domain的irq_data
                    │  └──────────────────┘  │
                    └────────────────────────┘
                                 │
                                 ▼
                    ┌────────────────────────┐
                    │   struct irq_domain    │
                    │  ┌──────────────────┐  │
                    │  │ *ops             │──┼──> struct irq_domain_ops (操作函数)
                    │  │ *parent          │──┼──> 层级: 父domain
                    │  │ *root            │──┼──> 根domain
                    │  │ revmap[]         │──┼──> hwirq -> irq_data 反向映射表
                    │  └──────────────────┘  │
                    └────────────────────────┘
```

例如这两个 irq ，其 irq_common_data 在 PCI-MSIX 这个层次是相同的，但是在 VECTOR 不同的，
而且可以发现在 VECTOR 层次 irq_data 和 irq_common_data 就是在一起的
```txt
🤒  ./drgn-wrapper.sh -c /proc/kcore irq_hierarchy.py 58
=== IRQ 层级分析: virq=58 ===
============================================================

irq_desc: 0xffff888112320800
virq: 58
action name: virtio0-request

------------------------------------------------------------
遍历 irq_data -> parent_data 链:
------------------------------------------------------------

[Level 0] Domain: PCI-MSIX-0000:00:02.0-12
  irq_data: 0xffff888112320858
  irq_commondata: 0xffff888112320868
  hwirq: 28
  chip: PCI-MSIX-0000:00:02.0
  domain flags: 0x213
    - IRQ_DOMAIN_FLAG_HIERARCHY

[Level 1] Domain: VECTOR - Root (no parent)
  irq_data: 0xffff888106cd6f40
  irq_commondata: 0xffff888106cd6f50
  hwirq: 58
  chip: APIC
  domain flags: 0x103
    - IRQ_DOMAIN_FLAG_HIERARCHY
```

```txt
🧀  ./drgn-wrapper.sh -c /proc/kcore irq_hierarchy.py 57
=== IRQ 层级分析: virq=57 ===
============================================================

irq_desc: 0xffff888112320400
virq: 57
action name: virtio0-request

------------------------------------------------------------
遍历 irq_data -> parent_data 链:
------------------------------------------------------------

[Level 0] Domain: PCI-MSIX-0000:00:02.0-12
  irq_data: 0xffff888112320458
  irq_commondata: 0xffff888112320468 ---<
  hwirq: 27
  chip: PCI-MSIX-0000:00:02.0
  domain flags: 0x213
    - IRQ_DOMAIN_FLAG_HIERARCHY

[Level 1] Domain: VECTOR - Root (no parent)
  irq_data: 0xffff888106cd6e80
  irq_commondata: 0xffff888106cd6e90
  hwirq: 57
  chip: APIC
  domain flags: 0x103
    - IRQ_DOMAIN_FLAG_HIERARCHY

```

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
