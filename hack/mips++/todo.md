# 问题

- bt tree 如何读去的 ?

- [ ] 中断的初始化
- [ ] ejtag 的使用

- [ ] 多核启动的时候，一个核启动，如何让第二个核启动?
- [ ] dmi / smbios / acpi / efi

- [ ] 忽然发现时钟系统有点麻烦
- [ ] arch/loongarch/kernel/vmlinux.lds.S 是如何被使用的 ?

## TODO
- [ ] 中断系统

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
    - init_initrd : 根据内核参数计算出来 initrd 的位置
    - prom_init
      - [ ] set_io_port_base : 这个不是硬件应该决定的吗? 还是说，硬件是这么决定，然后软件赢编码
      - efi_init
      - [ ] acpi_table_upgrade : 让人恐怖的 acpi 机制，但是 acpi 似乎本身是一个单独模块，想想办法将其 ac
      - [ ] acpi_boot_table_init : 这两个函数在 arch/loongarch/kernel/acpi/boot.c
      - [ ] acpi_boot_init
      - [ ] register_pch_pic : 应该是从 acpi 中间读去
      - [ ] prom_init_numa_memory : 从 acpi 中读去信息初始化 numa, 似乎即使是不支持 numa 也需要处理 numa 的配置信息 ?
      - [ ] dmi_scan_machine
      - [ ] dmi_set_dump_stack_arch_desc : 似乎解析了 dmi 信息之后，就可以获取 bios 的信息了
      - [ ] efi_runtime_init
      - [ ] register_smp_ops : 检查一下这些注册函数的使用位置
      - [ ] loongson_acpi_init : acpi 初始化，但是似乎之前就已经进行了 acpi 初始化
    - cpu_report : 显示一些输出信息
    - [ ] arch_mem_init : 存在一些 device tree 的初始化
    - resource_init : resources 子系统的初始化
    - plat_smp_setup : 调用函数 plat_smp_ops::smp_finish
    - [ ] cpu_cache_init : mips 特有的 cache 初始化，照抄就好了
    - [ ] paging_init :
      - pagetable_init
      - free_area_init_nodes : 内存系统初始化(1) 会调用到 page_alloc.c 中去, 然后进行整个子系统的初始化
    - [ ] boot_cpu_trap_init : 完成中断，tlb 相关的初始化，其他的 cpu 也需要调用一次整个函数吗?
- setup_command_line :  建立命令行参数。内核命令行参数可以写在启动配置文件 (boot.cfg 或 grub.cfg) 中，由BIOS 或者启动器传递给内核[^1]
- setup_per_cpu_areas : percpu 的空间分配
- [ ] build_all_zonelists : 内存系统初始化(2)
- page_alloc_init
  - [ ] page_alloc_cpu_dead : 内存系统初始化(3)
- [ ] parse_early_param : 又调用一次
- trap_init
- vfs_caches_init_early
- [ ] set_handler : general exception 的初始化，那么和前面 boot_cpu_trap_init 的关系是什么 ?
- mm_init : 好几个函数都是空的
  - kmem_cache_init
- sched_init
- [ ] early_irq_init : 在 trap_init 中没有初始化中断的入口，现在只是初始化了 irq_desc
  - init_irq_default_affinity
  - arch_probe_nr_irqs
  - alloc_desc
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
