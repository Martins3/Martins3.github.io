## 分析 rom 的热迁移行为
(这里讲了这么多，只是为了说明一下 size，实在是过分了)

其实 ROM 没有什么特殊的地方，除了热迁移的时候其中的 rom size

### info ramblock 的展示内容是什么

- RAMBlock::used_length
- RAMBlock::max_length

```c
GString *ram_block_format(void)
{
    RAMBlock *block;
    char *psize;
    GString *buf = g_string_new("");

    RCU_READ_LOCK_GUARD();
    g_string_append_printf(buf, "%24s %8s  %18s %18s %18s %18s %3s\n",
                           "Block Name", "PSize", "Offset", "Used", "Total",
                           "HVA", "RO");

    RAMBLOCK_FOREACH(block) {
        psize = size_to_str(block->page_size);
        g_string_append_printf(buf, "%24s %8s  0x%016" PRIx64 " 0x%016" PRIx64
                               " 0x%016" PRIx64 " 0x%016" PRIx64 " %3s\n",
                               block->idstr, psize,
                               (uint64_t)block->offset,
                               (uint64_t)block->used_length,
                               (uint64_t)block->max_length,
                               (uint64_t)(uintptr_t)block->host,
                               block->mr->readonly ? "ro" : "rw");

        g_free(psize);
    }

    return buf;
}
```

### 基本的观测实验

1. virsh qemu-monitor-command testvm --hmp 'info ramblock'


x86 4k :
```txt
(qemu) info ramblock
              Block Name    PSize              Offset               Used              Total                HVA  RO
                    mem0    4 KiB  0x0000000000000000 0x0000000100000000 0x0000000100000000 0x0000fffedbe00000  rw
                    mem1    4 KiB  0x0000000100000000 0x0000000100000000 0x0000000100000000 0x0000fffddbc00000  rw
             virt.flash0    4 KiB  0x0000000200000000 0x0000000004000000 0x0000000004000000 0x0000fffdcfe00000  rw
             virt.flash1    4 KiB  0x0000000204000000 0x0000000004000000 0x0000000004000000 0x0000fffdcbc00000  rw
 0000:00:0e.0/gpu-fb-mem    4 KiB  0x0000000208100000 0x0000000001000000 0x0000000001000000 0x0000fffdc2e00000  rw
    /rom@etc/acpi/tables    4 KiB  0x0000000209100000 0x0000000000020000 0x0000000000200000 0x0000fffdc8200000  ro
0000:00:02.0/virtio-net-pci.rom    4 KiB  0x0000000208040000 0x0000000000040000 0x0000000000040000 0x0000fffdd8200000  ro
0000:00:03.0/virtio-net-pci.rom    4 KiB  0x0000000208080000 0x0000000000040000 0x0000000000040000 0x0000fffdc8800000  ro
0000:00:04.0/virtio-net-pci.rom    4 KiB  0x00000002080c0000 0x0000000000040000 0x0000000000040000 0x0000fffdc8600000  ro
   /rom@etc/table-loader    4 KiB  0x0000000209300000 0x0000000000001000 0x0000000000010000 0x0000fffdc2c00000  ro
                  pvtime    4 KiB  0x0000000208000000 0x0000000000002000 0x0000000000002000 0x0000ffffe0200000  rw
      /rom@etc/acpi/rsdp    4 KiB  0x0000000209340000 0x0000000000001000 0x0000000000001000 0x0000fffdc2a00000  ro
```

arm 4k 机器:
```txt
(qemu) info ramblock
              Block Name    PSize              Offset               Used              Total                HVA  RO
                    mem0    4 KiB  0x0000000000000000 0x0000000100000000 0x0000000100000000 0x0000fffedbe00000  rw
                    mem1    4 KiB  0x0000000100000000 0x0000000100000000 0x0000000100000000 0x0000fffddbc00000  rw
             virt.flash0    4 KiB  0x0000000200000000 0x0000000004000000 0x0000000004000000 0x0000fffdcfe00000  rw
             virt.flash1    4 KiB  0x0000000204000000 0x0000000004000000 0x0000000004000000 0x0000fffdcbc00000  rw
 0000:00:0e.0/gpu-fb-mem    4 KiB  0x0000000208100000 0x0000000001000000 0x0000000001000000 0x0000fffdc2e00000  rw
0000:00:02.0/virtio-net-pci.rom    4 KiB  0x0000000208040000 0x0000000000040000 0x0000000000040000 0x0000fffdd8200000  ro
0000:00:03.0/virtio-net-pci.rom    4 KiB  0x0000000208080000 0x0000000000040000 0x0000000000040000 0x0000fffdc8800000  ro
0000:00:04.0/virtio-net-pci.rom    4 KiB  0x00000002080c0000 0x0000000000040000 0x0000000000040000 0x0000fffdc8600000  ro
                  pvtime    4 KiB  0x0000000208000000 0x0000000000002000 0x0000000000002000 0x0000ffffe0200000  rw
    /rom@etc/acpi/tables    4 KiB  0x0000000209100000 0x0000000000020000 0x0000000000200000 0x0000fffdc8200000  ro
   /rom@etc/table-loader    4 KiB  0x0000000209300000 0x0000000000001000 0x0000000000010000 0x0000fffdc2c00000  ro
      /rom@etc/acpi/rsdp    4 KiB  0x0000000209340000 0x0000000000001000 0x0000000000001000 0x0000fffdc2a00000  ro
```

arm 当内核变为 64k 页的时候，那么
```txt
(qemu) info ramblock
              Block Name    PSize              Offset               Used              Total                HVA  RO
                    mem0   64 KiB  0x0000000000000000 0x0000000100000000 0x0000000100000000 0x0000fffe67e00000  rw
                    mem1   64 KiB  0x0000000100000000 0x0000000100000000 0x0000000100000000 0x0000fffd67c00000  rw
             virt.flash0   64 KiB  0x0000000200000000 0x0000000004000000 0x0000000004000000 0x0000fffd63a00000  rw
             virt.flash1   64 KiB  0x0000000204000000 0x0000000004000000 0x0000000004000000 0x0000fffd5f800000  rw
 0000:00:0e.0/gpu-fb-mem   64 KiB  0x00000002080c0000 0x0000000001000000 0x0000000001000000 0x0000fffd28c00000  rw
0000:00:02.0/virtio-net-pci.rom   64 KiB  0x0000000208000000 0x0000000000040000 0x0000000000040000 0x0000fffd5c600000  ro
0000:00:03.0/virtio-net-pci.rom   64 KiB  0x0000000208040000 0x0000000000040000 0x0000000000040000 0x0000fffd5c400000  ro
0000:00:04.0/virtio-net-pci.rom   64 KiB  0x0000000208080000 0x0000000000040000 0x0000000000040000 0x0000fffd5c200000  ro
    /rom@etc/acpi/tables   64 KiB  0x00000002090c0000 0x0000000000020000 0x0000000000200000 0x0000fffd44c00000  ro
   /rom@etc/table-loader   64 KiB  0x00000002092c0000 0x0000000000010000 0x0000000000010000 0x0000fffd44a00000  ro
      /rom@etc/acpi/rsdp   64 KiB  0x0000000209300000 0x0000000000010000 0x0000000000010000 0x0000fffd44800000  ro
```

arm 16k 页:
```txt
󰂄 6% 🧀  rk -m
~/core/vn/code/qemu ~
~
ip   : 10.0.1
iso  : /home/martins3/hack/iso
vm   : /home/martins3/hack/aarch64
🦮   : /home/martins3/core/martins3
vm dir : /home/martins3/hack/aarch64/oe
vm dir : /home/martins3/hack/aarch64/oe
Choose interface:
QEMU 9.0.50 monitor - type 'help' for more information
(qemu) info ramblock
              Block Name    PSize              Offset               Used              Total                HVA  RO
                    mem0   16 KiB  0x0000000000000000 0x0000000200000000 0x0000000200000000 0x0000fffc86000000  rw
             virt.flash0   16 KiB  0x0000000200000000 0x0000000004000000 0x0000000004000000 0x0000fffc7a000000  rw
             virt.flash1   16 KiB  0x0000000204000000 0x0000000004000000 0x0000000004000000 0x0000fffc74000000  rw
0000:00:02.0/virtio-net-pci.rom   16 KiB  0x0000000208040000 0x0000000000040000 0x0000000000040000 0x0000fffeacc00000  ro
0000:00:03.0/virtio-net-pci.rom   16 KiB  0x0000000208080000 0x0000000000040000 0x0000000000040000 0x0000fffeaca00000  ro
0000:00:04.0/virtio-net-pci.rom   16 KiB  0x00000002080c0000 0x0000000000040000 0x0000000000040000 0x0000fffeac800000  ro
                  pvtime   16 KiB  0x0000000208000000 0x0000000000004000 0x0000000000004000 0x0000fffeace00000  rw
    /rom@etc/acpi/tables   16 KiB  0x0000000208100000 0x0000000000020000 0x0000000000200000 0x0000fffeac400000  ro
   /rom@etc/table-loader   16 KiB  0x0000000208300000 0x0000000000004000 0x0000000000010000 0x0000fffeac200000  ro
      /rom@etc/acpi/rsdp   16 KiB  0x0000000208340000 0x0000000000004000 0x0000000000004000 0x0000fffea7e00000  ro
```


### romsize 和 datasize 是什么关系?
似乎 romsize 中
rom_add_blob 似乎有两个类似的结构体
```c
    g_assert(rom->romsize >= rom->datasize);
```
看看从 rom 到 ram 的传递过程

rom_set_mr

```txt
    memory_region_init_resizeable_ram(rom->mr, owner, name,
                                      rom->datasize, rom->romsize,
                                      fw_cfg_resized,
                                      &error_fatal);
```


## 如何初始化的 max_length

qemu_ram_alloc_internal

就是这个位置:
```txt
#1  qemu_ram_alloc_internal (size=16384, size@entry=36, max_size=16384, max_size@entry=4096, resized=resized@entry=0xaaaaaaef6070 <fw_cfg_resized>, host=host@entry=0x0, ram_flags=ram_flags@entry=4, mr=mr@entry=0xaaaaacaa6770, errp=errp@entry=0xffffffffd2d0) at ../system/physmem.c:2110
#2  0x0000aaaaab542094 in qemu_ram_alloc_resizeable (size=size@entry=36, maxsz=maxsz@entry=4096, resized=resized@entry=0xaaaaaaef6070 <fw_cfg_resized>, mr=mr@entry=0xaaaaacaa6770, errp=errp@entry=0xffffffffd2d0) at ../system/physmem.c:2146
#3  0x0000aaaaab535070 in memory_region_init_resizeable_ram (mr=0xaaaaacaa6770, owner=owner@entry=0xaaaaada47c20, name=name@entry=0xffffffffd380 "/rom@etc/acpi/rsdp", size=36, max_size=4096, resized=resized@entry=0xaaaaaaef6070 <fw_cfg_resized>, errp=0xaaaaac9e1ed8 <error_fatal>) at ../system/memory.c:1596
#4  0x0000aaaaaaef6014 in rom_set_mr (rom=rom@entry=0xaaaab1f95ee0, owner=0xaaaaada47c20, name=name@entry=0xffffffffd380 "/rom@etc/acpi/rsdp", ro=ro@entry=true) at ../hw/core/loader.c:1046
#5  0x0000aaaaaaef74bc in rom_add_blob (name=name@entry=0xaaaaab7ea220 "etc/acpi/rsdp", blob=blob@entry=0xaaaaad012c40, len=36, max_len=max_len@entry=4096, addr=addr@entry=18446744073709551615, fw_file_name=fw_file_name@entry=0xaaaaab7ea220 "etc/acpi/rsdp", fw_callback=fw_callback@entry=0xaaaaab274be0 <virt_acpi_build_update>, callback_opaque=callback_opaque@entry=0xaaaaad1f97a0, as=<optimized out>, as@entry=0x0, read_only=<optimized out>, read_only@entry=true) at ../hw/core/loader.c:1169
#6  0x0000aaaaaaebaa18 in acpi_add_rom_blob (update=update@entry=0xaaaaab274be0 <virt_acpi_build_update>, opaque=opaque@entry=0xaaaaad1f97a0, blob=0xaaaaad1f33d0, name=name@entry=0xaaaaab7ea220 "etc/acpi/rsdp") at ../hw/acpi/utils.c:47
#7  0x0000aaaaab274e40 in virt_acpi_setup (vms=vms@entry=0xaaaaace0cdb0) at ../hw/arm/virt-acpi-build.c:1119
#8  0x0000aaaaab26c6bc in virt_machine_done (notifier=0xaaaaace0cf08, data=<optimized out>) at ../hw/arm/virt.c:1750
#9  0x0000aaaaab7a25dc in notifier_list_notify (list=list@entry=0xaaaaac9bfdc0 <machine_init_done_notifiers>, data=data@entry=0x0) at ../util/notify.c:39
#10 0x0000aaaaaaf002fc in qdev_machine_creation_done () at ../hw/core/machine.c:1640
#11 0x0000aaaaab1af17c in qemu_machine_creation_done (errp=0xaaaaac9e1ed8 <error_fatal>) at ../system/vl.c:2692
#12 qmp_x_exit_preconfig (errp=0xaaaaac9e1ed8 <error_fatal>) at ../system/vl.c:2722
#13 0x0000aaaaab1b2af0 in qemu_init (argc=<optimized out>, argv=<optimized out>) at ../system/vl.c:3766
#14 0x0000aaaaaae6afac in main (argc=<optimized out>, argv=<optimized out>) at ../system/main.c:47
```

变化发生在 qemu_ram_alloc_internal

```sh
    size = ROUND_UP(size, align);
    max_size = ROUND_UP(max_size, align);
```

一种想法是不要让 size 进行 align ，从而让 used_length 就是数据真实的大小。

但是 RAMBlock::used_length 是在 bitmap 计算中，是一定需要考虑的。

## 热迁移中，关于 size 的比较逻辑

比较逻辑在 qemu_ram_resize 中，这个东西

### 首先看 block->used_length

```txt
Size too large: /rom@etc/acpi/rsdp: 0x10000 > 0x1000: Invalid argument
```

```c
    if (block->max_length < newsize) {
        error_setg_errno(errp, EINVAL,
                         "Size too large: %s: 0x" RAM_ADDR_FMT
                         " > 0x" RAM_ADDR_FMT, block->idstr,
                         newsize, block->max_length);
        return -EINVAL;
    }
```
所以比较的是 `max_length`

### qemu_ram_resize 的调用路径

## 最后参考一下这两个问题
commit 50337286b796 ("acpi: Set proper maximum size for "etc/acpi/rsdp" blob")
commit 6c2b24d1d2b1 ("acpi: Set proper maximum size for "etc/table-loader" blob")
commit 6930ba0d44b2 ("acpi: Move maximum size logic into acpi_add_rom_blob()")

## 其他问题
为什么在 info mtree 中看不到
(到底看不到什么啊)

例如在 x86 上的机器:
```txt
    0000000000000600-000000000000067f (prio 0, i/o): ich9-pm
      0000000000000600-0000000000000603 (prio 0, i/o): acpi-evt
      0000000000000604-0000000000000605 (prio 0, i/o): acpi-cnt
      0000000000000608-000000000000060b (prio 0, i/o): acpi-tmr
      0000000000000620-000000000000062f (prio 0, i/o): acpi-gpe0
      0000000000000630-0000000000000637 (prio 0, i/o): acpi-smi
      0000000000000660-000000000000067f (prio 0, i/o): sm-tco
    0000000000000700-000000000000073f (prio 1, i/o): pm-smbus
    0000000000000a00-0000000000000a17 (prio 0, i/o): acpi-mem-hotplug
    0000000000000cc0-0000000000000cd7 (prio 0, i/o): acpi-pci-hotplug
    0000000000000cd8-0000000000000ce3 (prio 0, i/o): acpi-cpu-hotplug
```

arm 机器上:
```txt
    0000000009070000-0000000009070017 (prio 0, i/o): memhp container
      0000000009070000-0000000009070017 (prio 0, i/o): acpi-mem-hotplug
    0000000009080000-0000000009080003 (prio 0, i/o): acpi-ged
```

## 如何 QEMU 迁移任何东西
每个设备通过 VMStateDescription 声明其需要保存/恢复的状态（寄存器、缓冲区、内存等）。
对于 RAM 类内存，如果属于 ram_block（通过 memory_region_init_ram() 创建），QEMU 会在迁移过程中自动传输其内容（via ram_save_iterate）。
关键点：设备必须显式注册其状态，且其内存必须是 QEMU RAM 框架管理的（不是裸指针或 mmap 未注册区域）。

SaveVMHandlers

哦，就是这个东西了:

少数和具体设备无关的走这里，更多时候，通过 vmstate_register_ram 来分析:
```c
void vmstate_register_ram_global(MemoryRegion *mr)
{
    vmstate_register_ram(mr, NULL);
}
```

```txt
[martins3:vmstate_register_ram_global:3572] mem0
[martins3:vmstate_register_ram:3556] mem0
[martins3:vmstate_register_ram:3556] pc.bios
[martins3:vmstate_register_ram:3556] pc.rom
[martins3:vmstate_register_ram:3556] vga.vram
[martins3:vmstate_register_ram:3556] vga.rom
[martins3:vmstate_register_ram:3556] virtio-net-pci.rom
[martins3:vmstate_register_ram:3556] virtio-net-pci.rom
[martins3:virtio_dummy_instance_init:120] 0x56473f18b4e0
GPU instance init
GPU Realize
[martins3:vmstate_register_ram:3556] gpu-fb-mem
[martins3:vmstate_register_ram_global:3572] /rom@etc/acpi/tables
[martins3:vmstate_register_ram:3556] /rom@etc/acpi/tables
[martins3:vmstate_register_ram_global:3572] /rom@etc/table-loader
[martins3:vmstate_register_ram:3556] /rom@etc/table-loader
[martins3:vmstate_register_ram_global:3572] /rom@etc/acpi/rsdp
[martins3:vmstate_register_ram:3556] /rom@etc/acpi/rsdp
```
vmstate_register_ram 其实只是为了获取到一个 dev 的名称而已。

如果内存是热插进去的，那么就会使用:
```txt
[martins3:vmstate_register_ram:3556] hp_mem0
```

### 如果热插 + 热迁移

source 端有这个错误:
```txt
migrate_error error=Unable to write to socket: Broken pipe
qemu-system-x86_64: Unable to write to socket: Broken pipe
```
target 端错误为，显然在 target 端启动的时候，也是需要 hp_mem0 参数的，
但是现在没有:
```txt
qemu_loadvm_state_section_startfull 2(ram) 0 4
vmstate_load ram, (old)
ram_load_start
qemu-system-x86_64: Unknown ramblock "hp_mem0", cannot accept migration
qemu-system-x86_64: error while loading state for instance 0x0 of device 'ram'
qemu_loadvm_state_post_main -22
vmstate_downtime_checkpoint dst-precopy-loadvm-completed
process_incoming_migration_co_end ret=-22 postcopy-state=0
migrate_set_state new state failed
migrate_error error=load of migration failed: Invalid argument
multifd_recv_terminate_threads error 0
multifd_recv_thread_end channel 0 packets 1
multifd_recv_thread_end channel 2 packets 1
multifd_recv_thread_end channel 1 packets 1
multifd_recv_thread_end channel 3 packets 1
loadvm_state_cleanup
qemu_file_fclose
```

### 所以，总是可以添加一个 ramblock 的吗?

似乎是的，就是可以的:
memory_region_init_ram_from_fd

也就是 qemu_ram_alloc_from_fd 就是可以了，不过显然，我们可以更加简单一点。

vmstate_register_ram

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - tcp_chr_read
                  - monitor_read
                    - readline_handle_byte
                      - monitor_command_cb
                        - handle_hmp_command
                          - handle_hmp_command_exec
                            - handle_hmp_command_exec
                              - hmp_object_add
                                - user_creatable_add_from_str
                                  - user_creatable_add_qapi
                                    - user_creatable_add_type
                                      - user_creatable_complete
                                        - host_memory_backend_memory_complete
                                          - memfd_backend_memory_alloc
                                            - memory_region_init_ram_from_fd
                                              - qemu_ram_alloc_from_fd

参考这个也许是不错的，hw/display/vga.c

```c
    memory_region_init_ram_nomigrate(&s->vram, obj, "vga.vram", s->vram_size,
                                     &local_err);
    vmstate_register_ram(&s->vram, s->global_vmstate ? NULL : DEVICE(obj));
```
hw/tpm/tpm_ppi.c 也是可以作为参考的。

## 基本的测试
```txt
qemu-system-x86_64: Size mismatch: 0000:00:05.0/virtio-net-pci.rom: 0x1000 != 0x8000000: Invalid argument
qemu-system-x86_64: error while loading state for instance 0x0 of device 'ram'
qemu-system-x86_64: load of migration failed: Invalid argument
```

## resizable ROM

常规路径中:
- memory_region_init_rom
  - memory_region_init_rom_nomigrate
    - memory_region_init_ram_flags_nomigrate
- memory_region_init_ram

比较有趣的是，在 ARM 环境中 中才会调用 memory_region_init_rom

实际上，就是我们常规简单的 ROM 使用 resizable，并没有什么奇怪，但是对于 PCI 的 ROM 并没有这种操作，还是有点奇怪的:
- rom_add_blob
- rom_add_file
  - rom_set_mr
    - memory_region_init_resizeable_ram

```c
ssize_t rom_add_option(const char *file, int32_t bootindex)
{
    return rom_add_file(file, "genroms", 0, bootindex, true, NULL, NULL);
}
```

- pci_add_option_rom 中
  - 如果 pci 设备不支持 ROM bar ，使用 fw_cfg 的方式加载，那么就可以的，从很早之前，这些代码都是没用的。

## 看懂这个 thread
https://lists.gnu.org/archive/html/qemu-devel/2020-02/msg03626.html

## qemu_ram_resize : 处理 resize 机制

resizeable ram 提出来的时候 patch
```diff
History:        #0
Commit:         60786ef33928332ccef21f99503f56d781fade0c
Author:         Michael S. Tsirkin <mst@redhat.com>
Author Date:    Mon 17 Nov 2014 06:24:36 AM CST
Committer Date: Thu 08 Jan 2015 07:17:55 PM CST

memory: API to allocate resizeable RAM MR

Add API to allocate resizeable RAM MR.

This looks just like regular RAM generally, but
has a special property that only a portion of it
(used_length) is actually used, and migrated.

This used_length size can change across reboots.

Follow up patches will change used_length for such blocks at migration,
making it easier to extend devices using such RAM (notably ACPI,
but in the future thinkably other ROMs) without breaking migration
compatibility or wasting ROM (guest) memory.

Device is notified on resize, so it can adjust if necessary.

Note: nothing prevents making all RAM resizeable in this way.
However, reviewers felt that only enabling this selectively will
make some class of errors easier to detect.

Signed-off-by: Michael S. Tsirkin <mst@redhat.com>
Reviewed-by: Paolo Bonzini <pbonzini@redhat.com>
```

- fw_cfg_resized : 只是更改一下，给 firmware config 做个修改

## 不要在热迁移的时候动态修改源端的 size

```diff
commit c7c0e72408df5e7821c0e995122fb2fe0ac001f1
Author: David Hildenbrand <david@redhat.com>
Date:   Thu Apr 29 13:27:02 2021 +0200

    migration/ram: Handle RAM block resizes during precopy

    Resizing while migrating is dangerous and does not work as expected.
    The whole migration code works on the usable_length of ram blocks and does
    not expect this to change at random points in time.

    In the case of precopy, the ram block size must not change on the source,
    after syncing the RAM block list in ram_save_setup(), so as long as the
    guest is still running on the source.

    Resizing can be trigger *after* (but not during) a reset in
    ACPI code by the guest
    - hw/arm/virt-acpi-build.c:acpi_ram_update()
    - hw/i386/acpi-build.c:acpi_ram_update()

    Use the ram block notifier to get notified about resizes. Let's simply
    cancel migration and indicate the reason. We'll continue running on the
    source. No harm done.

    Update the documentation. Postcopy will be handled separately.

    Reviewed-by: Peter Xu <peterx@redhat.com>
    Signed-off-by: David Hildenbrand <david@redhat.com>
    Message-Id: <20210429112708.12291-5-david@redhat.com>
    Signed-off-by: Dr. David Alan Gilbert <dgilbert@redhat.com>
      Manual merge
```

在 ram_mig_ram_block_resized 中的代码中，增加检查的 hook
```c
static RAMBlockNotifier ram_mig_ram_notifier = {
    .ram_block_resized = ram_mig_ram_block_resized,
};
```

- 如何实现在 ram_mig_ram_block_resized 中检测，然后造成失败啊?
  - [ ] 猜测 migraiton 的过程中，source 端有时候会成为 active 的，当 ram 被修改大小，
那么，就会调用到这个 notifier 的。

## 为什么会出现 guest 天生的修改这些 memory region 的大小

guest 中主动进行 resize 的场景：
- fw_cfg_acpi_mr_restore_post_load -> fw_cfg_update_mr
- acpi_ram_update
  - memory_region_ram_resize
    - qemu_ram_resize


```txt
$ p mr.name
$1 = 0x555556948dc0 "/rom@etc/acpi/tables"
$2 = 0x555556de3a80 "/rom@etc/acpi/rsdp"
$3 = 0x55555696b740 "/rom@etc/table-loader"
```
不去修改这些 memory region 会出现什么问题吗?

```txt
#0  acpi_ram_update (mr=0x555557a45ae0, data=0x555556905390) at ../hw/i386/acpi-build.c:2608
#1  0x0000555555a9addd in acpi_build_update (build_opaque=0x555557a88d70) at ../hw/i386/acpi-build.c:2632
#2  acpi_build_update (build_opaque=0x555557a88d70) at ../hw/i386/acpi-build.c:2617
#3  0x000055555593cdcc in fw_cfg_select (s=s@entry=0x555556a1eb30, key=key@entry=42) at ../hw/nvram/fw_cfg.c:285
#4  0x000055555593d1d5 in fw_cfg_dma_transfer (s=0x555556a1eb30) at ../hw/nvram/fw_cfg.c:359
#5  0x0000555555b79dc0 in memory_region_write_accessor (mr=mr@entry=0x555556a1eec0, addr=4, value=value@entry=0x7ffde3bfe518, size=size@entry=4, shift=<optimized out>, mask=mask@entry=4294967295, attrs=...) at ../softmmu/memory.c:493
#6  0x0000555555b77616 in access_with_adjusted_size (addr=addr@entry=4, value=value@entry=0x7ffde3bfe518, size=size@entry=4, access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=0x555555b79d40 <memory_region_write_accessor>, mr=0x555556a1eec0, attrs=...) at ../softmmu/memory.c:550
#7  0x0000555555b7b86a in memory_region_dispatch_write (mr=mr@entry=0x555556a1eec0, addr=4, data=<optimized out>, op=<optimized out>, attrs=attrs@entry=...) at ../softmmu/memory.c:1522
#8  0x0000555555b82ba0 in flatview_write_continue (fv=fv@entry=0x7ffdd46aca50, addr=addr@entry=1304, attrs=..., attrs@entry=..., ptr=ptr@entry=0x7ffff53f0000, len=len@entry=4, addr1=<optimized out>, l=<optimized out>, mr=0x555556a1eec0) at /home/martins3/core/qemu/include/qemu/host-utils.h:166
#9  0x0000555555b82e60 in flatview_write (fv=0x7ffdd46aca50, addr=addr@entry=1304, attrs=attrs@entry=..., buf=buf@entry=0x7ffff53f0000, len=len@entry=4) at ../softmmu/physmem.c:2870
#10 0x0000555555b865f9 in address_space_write (len=4, buf=0x7ffff53f0000, attrs=..., addr=1304, as=0x55555659cce0 <address_space_io>) at ../softmmu/physmem.c:2966
#11 address_space_rw (as=0x55555659cce0 <address_space_io>, addr=addr@entry=1304, attrs=attrs@entry=..., buf=0x7ffff53f0000, len=len@entry=4, is_write=is_write@entry=true) at ../softmmu/physmem.c:2976
#12 0x0000555555c08d3b in kvm_handle_io (count=1, size=4, direction=<optimized out>, data=<optimized out>, attrs=..., port=1304) at ../accel/kvm/kvm-all.c:2639
#13 kvm_cpu_exec (cpu=cpu@entry=0x5555568ea120) at ../accel/kvm/kvm-all.c:2890
#14 0x0000555555c0a1ad in kvm_vcpu_thread_fn (arg=arg@entry=0x5555568ea120) at ../accel/kvm/kvm-accel-ops.c:51
#15 0x0000555555d790b9 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:505
#16 0x00007ffff6a88e86 in start_thread () from /nix/store/4nlgxhb09sdr51nc9hdm8az5b08vzkgx-glibc-2.35-163/lib/libc.so.6
#17 0x00007ffff6b0fc60 in clone3 () from /nix/store/4nlgxhb09sdr51nc9hdm8az5b08vzkgx-glibc-2.35-163/lib/libc.so.6
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
