## QEMU 初始化过程分析
基于 v4.1.0，但是对于 v6.0 也很有参考意义

<!-- vim-markdown-toc GitLab -->

* [main](#main)
* [tcg_init](#tcg_init)
* [x86_cpu_realizefn](#x86_cpu_realizefn)
* [i440fx_init](#i440fx_init)
* [notes](#notes)
  * [memory](#memory)
  * [tcg](#tcg)

<!-- vim-markdown-toc -->

## main
一下的代码从 main 开始，首先会经过一大段的参数解析的代码

- select_machine() : select_machine 中获取 MachineClass, 在这里抉择是 pc 还是 q35
- cpu_exec_init_all :
    - io_mem_init : 初始化 `MemoryRegion` `io_mem_unassigned`, 用于捕获访问 io 空洞的行为, 实际上这个 mr 永远都不会被使用,
    - [memory_map_init](#memory) : 初始化 `system_memory` 和 `io_memory` 这两个分别是两个对顶级的 container 分别和 address_space_io 和 address_space_memory 关联起来
- page_size_init
- configure_accelerator
    - accel_init_machine
      - AccelClass::init_machine : 在 tcg_accel_class_init 初始化
        - 实际上调用就是 [tcg_init](#tcg_init)
- machine_run_board_init
  - `machine_class->init` : DEFINE_I440FX_MACHINE 这个封装出来 pc_init_v6_1 来调用
    - pc_init1
      - x86_cpus_init : 多次调用 x86_cpu_new 来创建新的 CPU
        - x86_cpu_new
          - qdev_realize : 经过 QOM 的 object_property 机制，最后调用到 device_set_realized
            - device_set_realized
              - [x86_cpu_realizefn](#x86_cpu_realizefn)
      - [pc_memory_init](#memory)
        - e820_add_entry
        - pc_system_firmware_init : 处理 `-drive if=pflash` 的选项
          - x86_bios_rom_init : 不考虑 pflash, 这是唯一的调用者
            - memory_region_init_ram(bios, NULL, "pc.bios", bios_size, &error_fatal)
            - rom_add_file_fixed
              - rom_add_file
                - rom_insert
                - add_boot_device_path
            - map the last 128KB of the BIOS in ISA space
            - map all the bios at the top of memory
        - memory_region_init_ram(option_rom_mr, NULL, "pc.rom", PC_ROM_SIZE, &error_fatal);
        - fw_cfg_arch_create : 创建 `FWCfgState *fw_cfg`, 并且初始化 e820 CPU 数量之类的参数
        - rom_set_fw : 用从 fw_cfg_arch_create 返回的值初始化全局 fw_cfg
        - x86_load_linux : 如果指定了 kernel, 那么就从此处 load kernel
        - rom_add_option : 添加 rom 镜像，关于 rom 分析看 [loader](#loader)
      - pc_guest_info_init : 初始化 PCMachineState 的成员，注册上 pc_machine_done 最后执行
      - smbios_set_defaults : 初始化一些 smbios 变量，为下一步制作 smbios table 打下基础
      - pc_gsi_create : 创建了 qemu_irq，分配了 GSIState , 但是 GSIState 没有被初始化. X86MachineState::gsi 指向这个位置
      - [i440fx_init](#i440fx_init)
      - piix3_create
        - pci_create_simple_multifunction : 创建出来设备
        - 设置从 piix3 到 i440fx 的中断路由之类的事情
      - isa_bus_irqs
        - ISABus::irqs = X86MachineState::gsi
      - pc_i8259_create : 根据配置，存在多种
        - i8259_init
          - i8259_init_chip : 调用两次，分别初始化两个 irq chip 的
      - ioapic_init_gsi
      - x86_register_ferr_irq
      - pc_vga_init
        - pci_vga_init
        - isa_vga_init
      - pc_basic_device_init
        - ioport80_io 初始化
        - ioportF0_io 初始化
        - hpet 初始化 : hpet 不是
        - mc146818_rtc_init : 通过 QOM 调用 rtc_class_initfn 和 rtc_realizefn 之类的，进行 rtc 的初始化
        - i8254_pit_init
        - i8257_dma_init
        - pc_superio_init : https://en.wikipedia.org/wiki/Super_I/O
      - pc_nic_init : 网卡的初始化
      - pci_ide_create_devs
        - ide_drive_get
        - ide_create_drive
      - pc_cmos_init
        - 多次调用 rtc_set_memory 初始化 RTCState::cmos_data
      - piix4_pm_init : 当支持 acpi 的时候, 那么初始化电源管理
- soundhw_init
- parse_fw_cfg : 解析参数 -fw_cfg (Add named fw_cfg entry with contents from file file.)
- usb_parse
- device_init_func : 解析参数 -device 比如 nvme
- qdev_machine_creation_done
  - notifier_list_notify : 通过 qemu_add_machine_init_done_notifier 的 references 最终需要执行的 hook 了
    - pc_machine_done
      - rtc_set_cpus_count
      - acpi_setup
        - 依赖于 acpi 的 `x86ms->fw_cfg` 和 pcms->acpi_build_enabled, 否则都会失败
      - fw_cfg_build_smbios
    - x86_cpu_machine_done : 初始化 smram
    - ioapic_machine_done_notify : 用于支持 kvm 的 splitirq 的
    - pcibus_machine_done : pci address space 和 pci dev 的地址空间初始化
    - piix4_pm_machine_ready : piix4 pm 的 pci 配置空间
    - machine_init_notify
- qemu_system_reset
  - cpu_synchronize_all_states
  - MachineClass::reset => pc_machine_reset
    - qemu_devices_reset : 这会 reset 通过 qemu_register_reset 注册上的所有 devices
      - x86_cpu_machine_reset_cb
        - cpu_reset : 初始化 CPUX86State
      - rom_reset : 将 rom 拷贝到指定的空间上
      - acpi_build_reset
      - rtc_reset
      - pc_cmos_init_late
      - piix3_reset : 初始化 piix3 的配置空间
    - device_reset(X86CPU::apic_state)
  - cpu_synchronize_all_post_reset
- accel_setup_post
- os_setup_post
- main_loop
  - main_loop_wait
    - os_host_main_loop_wait
      - qemu_poll_ns

## tcg_init
- [tcg_init](#tcg)
  - tcg_exec_init
    - cpu_gen_init
      - tcg_context_init : 在 xqm 下这个没有意义
    - page_init : 初始化
      - page_size_init
      - page_table_config_init
    - tb_htable_init
    - code_gen_alloc
      - alloc_code_gen_buffer
        - alloc_code_gen_buffer_anon
    - tcg_prologue_init
  - tcg_region_init

## x86_cpu_realizefn
- x86_cpu_realizefn
  - qemu_register_reset(x86_cpu_machine_reset_cb, cpu);
  - cpu_exec_realizefn
    - cpu_list_add : 将 CPUState 添加到 cpus 中
    - CPUClass::tcg_initialize => tcg_x86_init
    - tlb_init
      - tlb_mmu_init
  - x86_cpu_expand_features
  - x86_cpu_filter_features
  - mce_init : machine check exception, 初始化之后，那些 helper 就可以正确工作了
  - qemu_init_vcpu : 创建执行线程
    - rr_cpu_thread_fn : 进行一些基本的注册工作，然后等待, 注意，此时在另一个线程中间了
      - [tcg_register_thread](#tcg)
  - [cpu_address_space_init](#memory)
  - x86_cpu_apic_realize
    - 通过 QOM 调用到 apic_common_realize
       - 通过 QOM 调用 apic_realize
    - 添加对应的 memory region
  - X86CPUClass::parent_realize : 也就是 cpu_common_realizefn, 这里并没有做什么事情

## i440fx_init
- `i440fx_init` : 只有 `pcmc->pci_enabled` 才会调用的
  - `qdev_new`("i440FX-pcihost") : 这当然会调用 i440fx_pcihost_initfn 和 i440fx_pcihost_class_init 之类的函数
    - `i440fx_pcihost_initfn` : 初始化出来 0xcf8 0xcfb 这两个关键地址
  - `pci_root_bus_new` : 创建 PCIBus
    - [ ] PCIHostState 和分别是啥关系 ? host bridge 和 bus 的关系 ?
    - `qbus_create("pci")`
      - `qbus_create("pci")`
        - `pci_root_bus_init`
          - 一些常规的初始化
          - `pci_host_bus_register` : 将 PCIHostState 挂载到一个全局的链表上
      - `qbus_init`
    - `pci_root_bus_init`
  - [ ] 处理 PCI 的地址空间的映射初始化
  - `init_pam`

## notes

### memory
Memory 初始化一共三个位置:
1. `memory_map_init`
2. `pc_memory_init` : 创建了两个 mr alias，ram_below_4g 以及 ram_above_4g，这两个 mr 分别指向 ram 的低 4g 以及高 4g 空间，这两个 alias 是挂在根 system_memory mr 下面的
创建 pc.bios
3. `cpu_address_space_init` : tcg 需要创建出来 CPUAddressSpace

### tcg
tcg 初始化两个部分:

- `tcg_init` : 初始化所有 tcg 相关基础设施
- `tcg_register_thread` : 分配 tcg region 给一个 CPU 使用
