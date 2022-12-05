## 如何设置不同的大小
```txt
qemu-system-x86_64: Size mismatch: 0000:00:05.0/virtio-net-pci.rom: 0x1000 != 0x8000000: Invalid argument
qemu-system-x86_64: error while loading state for instance 0x0 of device 'ram'
qemu-system-x86_64: load of migration failed: Invalid argument
```

- memory_region_init_rom
  - memory_region_init_rom_nomigrate
    - memory_region_init_ram_flags_nomigrate

- memory_region_init_ram

## 到底为什么存在

好吧，声明样的设备是 resizable 的

实际上，就是我们常规简单的 ROM 而已，并没有什么奇怪，但是对于 PCI 的 ROM 并没有这种操作，所以非常奇怪的:
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

- pci_add_option_rom
  - 如果 pci 设备不支持 ROM bar ，使用 fw_cfg 的方式加载，那么就可以的，从很早之前，这些代码都是没用的。

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

- [ ] QEMU : 的含义 c7c0e72408df5e7821c0e995122fb2fe0ac001f1

- [ ] resize 的含义是什么?
  - 直接注册好吗?
  - [ ] 其他人的处理方法是什么?
  - [ ] 其他函数的方法的操作的是什么?
  - [ ] 会出现引用计数的增加吗?

- fw_cfg_resized : 只是更改一下，给 firmware config 做个修改

- [ ] qemu_ram_resize : 这个逻辑没有完全看懂哇!

```c
    if (block->used_length == newsize) {
        /*
         * We don't have to resize the ram block (which only knows aligned
         * sizes), however, we have to notify if the unaligned size changed.
         */
        if (unaligned_size != memory_region_size(block->mr)) {
            memory_region_set_size(block->mr, unaligned_size);
            if (block->resized) {
                block->resized(block->idstr, unaligned_size, block->host);
            }
        }
        return 0;
    }
```
qemu_ram_resize 中是不会检查当前的 size

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
- [ ] 所以，到底是会出现什么问题呀?

- 但是，因为当初在 resizable 检查的时候，就被发现有问题啊！

- 因为对于这个问题，所以:
  - ram_mig_ram_block_resized : 就是用于检查 migration 的

- 但是他说的情况是如何触发的，无法理解。

## guest 中主动进行 resize 的时候
- fw_cfg_acpi_mr_restore_post_load -> fw_cfg_update_mr
- acpi_ram_update
  - memory_region_ram_resize
    - qemu_ram_resize

- 有点难理解，为什么会出现 guest 天生的修改这些 memory region 的大小：

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
