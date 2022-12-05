## poweroff 内核的触发过程

在 /etc/acpi/PWRF/00000080 的一个脚本文件，只有一行 `poweroff`

- 那么在 poweroff 如何实现的 ?
  - [ ] 执行 /sbin/poweroff, 但是 /sbin/poweroff 无法被 strace

acpi_pm1_cnt_write
```txt
#0  acpi_pm1_cnt_write (val=1, ar=0x555557b97d00) at ../hw/acpi/core.c:602
#1  acpi_pm_cnt_write (opaque=0x555557b97d00, addr=0, val=1, width=2) at ../hw/acpi/core.c:602
#2  0x0000555555b8d820 in memory_region_write_accessor (mr=mr@entry=0x555557b97f30, addr=0, value=value@entry=0x7fffd9ff90a8, size=size@entry=2, shift=<optimized out>,
```


内核调用流程:
- machine_power_off
  - native_machine_power_off
    - pm_power_off = sleep.c:acpi_power_off
      - acpi_enter_sleep_state
        - acpi_hw_legacy_sleep
          - acpi_pm1_cnt_write : 第一次执行写入到 0x1
          - acpi_pm1_cnt_write : 第二次执行写入 0x2001

在 acpi_pm1_cnt_write 中正好需要处理 ACPI_BITMASK_SLEEP_ENABLE

// acpi_hw_write 写入的地址正好是 604 啊
```txt
#0  acpi_hw_write (value=1, reg=0xffffffff82d0114c <acpi_gbl_FADT+172>) at drivers/acpi/acpica/hwregs.c:291
#1  0xffffffff8148e293 in acpi_hw_write_pm1_control (pm1a_control=<optimized out>, pm1b_control=pm1b_control@entry=1) at drivers/acpi/acpica/hwregs.c:462
#2  0xffffffff8148e649 in acpi_hw_legacy_sleep (sleep_state=<optimized out>) at drivers/acpi/acpica/hwsleep.c:101
#3  0xffffffff8108baa6 in __do_sys_reboot (magic1=-18751827, magic2=<optimized out>, cmd=1126301404, arg=0x0 <fixed_percpu_data>) at kernel/reboot.c:364
#4  0xffffffff81b945d0 in do_syscall_64 (nr=<optimized out>, regs=0xffffc90002007f58) at arch/x86/entry/common.c:47
#5  0xffffffff81c0007c in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:112
```
