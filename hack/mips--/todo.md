# Find out
代码主要出现的地方:
1. 内核

ptrace.c                                      96             51            600
signal.c                                     120             96            594
fpu.S                                         56            130            587

2. la64
3. mem

question:
- [ ] 内存的如何收集的
- [ ] dtb
- [ ] relocate.c 是怎么处理的?
- [ ] /sys/firmware
- [ ] https://wiki.gentoo.org/wiki/IOMMU_SWIOTLB
- [ ] /home/maritns3/core/loongson-dune/la-4.19/arch/loongarch/la64/env.c : BIOS 参数解析
- [ ] /home/maritns3/core/loongson-dune/la-4.19/arch/loongarch/la64/init.c : PROM ???
- [ ] /home/maritns3/core/loongson-dune/la-4.19/arch/loongarch/la64/irq.c : 存在一些 vi 中断等东西
- [ ] MSI 是怎么处理的 ?

```
[    0.000000] PCH_PIC[0]: pch_pic_id 0, version 0, address 0xe0010000000, IRQ 64-127

[    0.000000] irq: Added domain unknown-1
[    0.000000] irq: irq_domain_associate_many(<no-node>, irqbase=50, hwbase=0, count=14)
[    0.000000] irq: Added domain irqchip@(____ptrval____)
[    0.000000] Support EXT interrupt.
[    0.000000] irq: Added domain irqchip@(____ptrval____)
[    0.000000] irq: Added domain irqchip@(____ptrval____)
[    0.000000] irq: Added domain irqchip@(____ptrval____)
[    0.000000] irq: Added domain irqchip@(____ptrval____)
[    0.000000] irq: Added domain unknown-2
[    0.000000] irq: irq_domain_associate_many(<no-node>, irqbase=0, hwbase=0, count=16)
```

怀疑这个 irqbase 是最开始的索引，


# 问题
- bt tree 如何读去的 ?

- [ ] 有没有什么 pcie 上的特殊处理啊 ?
- [ ] ejtag 的使用

- [ ] 多核启动的时候，一个核启动，如何让第二个核启动?
- [ ] dmi / smbios / acpi / efi

- [ ] 忽然发现时钟系统有点麻烦
- [ ] arch/loongarch/kernel/vmlinux.lds.S 是如何被使用的 ?

- [ ] 最开始的时候，内核是被加载什么位置，让其可以在直接映射的地址空间的

- [ ] 应该分析一下  -M ls3a5k 

- [ ] vint 的内容是什么？

- [ ] 似乎中断就是只是两个数值，并不是所有的号码都用上了
- [ ] ipi 是怎么配置的，如果不小心将消息转发出去了，那岂不是很尴尬
- [ ] irq_common_data 来实现 irq 的 affinility
- [ ] 从这个函数的计算 set_vi_handler, 分析一下 vectored interrupt 是干什么的?

- ip2_irqdispatch
  - do_IRQ
    - generic_handle_irq
      - generic_handle_irq_desc
        - irq_desc::handle_irq

- extioi_irq_dispatch : 本身就是在中断上下文中间，现在在做下一级的跳转, TODO 问题是，怎么知道是从上一级跳转到下
  - irq_linear_revmap : 通过 irq 查询到
  - generic_handle_irq

- [ ] 到底存在那些 irq domain

- [ ] 真正让人恐惧的是， acpi_bus_init_irq 之类的初始化实际上是在 rest_init 后面




## pch 的初始化

- [ ] 从什么位置 ? bios / acpi ??

register_pch_pic

## TODO
- [ ] 中断系统初始化
- [ ] 参数解析到底怎么回事
  - prom_init_cmdline
  - setup_command_line

## notes
- 设备树信息可能被编译到内核中间，可能在通过 bios 传递过来的
- extioi_vec_init 中描述，所有的外部中断都是经过 extioi 进行的

## 内核启动的逐步分析
- arch/loongarch/kernel/head.S
  - 存储 fw 参数，初始化 kernel, 关键的寄存器初始化
  - [ ] fw_arg0 为什么从汇编中可以直接访问
- setup_arch
  - 到目前为止，大多数函数都是空的
  - [ ] setup_early_printk : 利用 serial8250 输出，easy
  - cpu_probe : cpuinfo_loongarch 的初始化, 记录一些信息
  - plat_early_init
    - setup_8250_early_printk_port
    - fw_init_cmdline : 利用 head.S 中的 fw_arg0 组装 kenrel cmdline
    - [ ] prom_init_env : 初始化之后会用到的数值
      - loongson_regaddr_set : 对于 prom 一些信息的初始化
      - list_find : 根据 fw 提供的参数, 解析信息, 初始化三个指针
        - parse_mem
        - parse_vbios
        - parse_screeninfo
      - memblock_and_maxpfn_init : 给 memblock 初始化物理内存
    - [ ] parse_cmdline
      - [ ] arch_mem_addpart : 物理内存会因为 `_text` 之类的符号地址而修改，为什么之前不去搞定
      - print_memory_map
      - parse_early_param
        - parse_early_options
          - [ ] parse_args : 应该是内核参数
    - init_initrd : 根据内核参数计算出来 initrd 的位置, 然后通过 finalize_initrd 在 memblock 中间预留空间
    - prom_init
      - [ ] set_io_port_base : 这个不是硬件应该决定的吗? 还是说，硬件是这么决定，然后软件赢编码
      - efi_init
      - [ ] acpi_table_upgrade : 让人恐怖的 acpi 机制，但是 acpi 似乎本身是一个单独模块，想想办法将其 ac
      - [ ] acpi_boot_table_init : 这两个函数在 arch/loongarch/kernel/acpi/boot.c
      - acpi_boot_init
        - [ ] 初始化 arch_acpi_wakeup_start, 但是不知道是做什么用的
        - acpi_process_madt : MADT(Multiple APIC Description Table)
          - [ ] acpi_parse_madt_lapic_entries : 进一步调用 acpi 的标准接口, 在 drivers/acpi/tables.c
          - acpi_parse_madt_pch_pic_entries
            - acpi_table_parse_madt : 调用过程中，会将 acpi_parse_pch_pic 作为参数, 后者进一步调用 register_pch_pic
      - register_pch_pic : 从 dmesg 看，acpi_boot_init 的调用路径下没有注册上 pic，这是唯一的调用位置
        - `[    0.000000] PCH_PIC[0]: pch_pic_id 0, version 0, address 0xe0010000000, IRQ 64-127`
      - [ ] prom_init_numa_memory : 从 acpi 中读去信息初始化 numa, 似乎即使是不支持 numa 也需要处理 numa 的配置信息 ?
      - [ ] dmi_scan_machine
      - [ ] dmi_set_dump_stack_arch_desc : 似乎解析了 dmi 信息之后，就可以获取 bios 的信息了
      - [ ] efi_runtime_init
      - [ ] register_smp_ops : 检查一下这些注册函数的使用位置
      - [ ] loongson_acpi_init : acpi 初始化，但是似乎之前就已经进行了 acpi 初始化
    - cpu_report : 显示一些输出信息
    - [ ] arch_mem_init : 存在一些 device tree 的初始化
      - early_init_dt_verify : 校验 initial_boot_params (也就是 dtb 的地址 loongson_fdt_blob)
      - unflatten_and_copy_device_tree : 解析和初始化设备树
    - resource_init : resources 子系统的初始化
    - plat_smp_setup : 调用函数 plat_smp_ops::smp_finish
    - [ ] cpu_cache_init : mips 特有的 cache 初始化，照抄就好了
    - [ ] paging_init :
      - pagetable_init
      - free_area_init_nodes : 内存系统初始化(1) 会调用到 page_alloc.c 中去, 然后进行整个子系统的初始化
    - boot_cpu_trap_init : 完成中断，tlb 相关的初始化
      - 这是 boot 的 cpu 调用的版本，还有一个 nonboot_cpu_trap_init
      - 完成 handler 空间的分配，ebase / refill_ebase / merror_ebase 的初始化
      - tlb_init : tlb handler 的构建以及其他的初始化
- setup_command_line :  建立命令行参数。内核命令行参数可以写在启动配置文件 (boot.cfg 或 grub.cfg) 中，由BIOS 或者启动器传递给内核[^1]
- setup_per_cpu_areas : percpu 的空间分配
- [ ] build_all_zonelists : 内存系统初始化(2)
- page_alloc_init
  - [ ] page_alloc_cpu_dead : 内存系统初始化(3)
- [ ] parse_early_param : 又调用一次
- trap_init : 调用 set_handler 将各种 exception 入口加以初始化
- vfs_caches_init_early
- mm_init : 好几个函数都是空的
  - kmem_cache_init
- sched_init
- [ ] early_irq_init
  - [ ] init_irq_default_affinity : 最后其作用体现在
  - alloc_desc : 分配了 16 个 irq_desc
  - irq_insert_desc : 插入到 irq_desc_tree 中，而这个是中断最开始的访问的 irq_desc
    - 与之配套的函数 : irq_to_desc 用于从 irq 找到对应的 irq
- init_IRQ
  - irq_set_noprobe
  - arch_init_irq
    - [ ] setup_IRQ : 这里应该是建立了三个 IRQ 中断控制器的处理
      - loongarch_cpu_irq_init
      - liointc_init
        - "Loongson Local I/O Interrupt Controller"
      - extioi_vec_init
        - [ ] irq_domain_alloc_fwnode : domain handle 是怎么回事, 可以通过 domain handle 创建 irq domain
        - irq_domain_create_linear : 
        - extioi_init : 初始化一下中断控制器的状态
        - irq_set_chained_handler_and_data : 将 `LOONGSON_BRIDGE_IRQ` 作为自己的 parent_irq 来传递到这里
          - [ ] 那么，上面一层的 irq_domain 是怎么知道其 handler 是这个，或者说，ip3_irqdispatch 向下调用的时候，其 irq_desc 的 handler 是怎么注册的
        - pch_msi_init
      - [ ] of_setup_pch_irqs : 这个应该也是会创建一个 domain
    - setup_vi_handler
      - 将中断的入口进行初始化
      - [ ] 为什么只有 ip0 /ip1 没有其他的，其他的中断入口也被屏蔽了
      - [ ] 而且，只有这两个入口，那么多的中断号，怎么搞
- timekeeping_init
- time_init
- [ ] perf_event_init
- [ ] call_function_init
- console_init
  - n_tty_init
  - [ ] initcall_from_entry : 似乎有一些调用通过 section 注册的函数的操作
- setup_per_cpu_pageset : 应该是 percpu page 设置
- numa_policy_init 
- [ ] acpi_early_init : 调用这三个函数都是 acpi 库提供的标准函数，这些都是 acpi 的初始化，那么之后使用在哪里?
  - acpi_reallocate_root_table
  - acpi_initialize_subsystem
  - acpi_load_tables
- [ ] sched_clock_init
- vfs_caches_init : 前面初始化了 vfs_caches_init 的内容
- pagecache_init
- acpi_subsystem_init
- rest_init

[^1]: P66
