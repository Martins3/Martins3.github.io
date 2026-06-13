## kdump
使用 kdump 机制
- centos 8 : https://unixcop.com/install-and-configure-kernel-crash-dump-on-centos-8/
- debian : https://www.cyberciti.biz/faq/how-to-on-enable-kernel-crash-dump-on-debian-linux/
- ubuntu : https://www.ebpf.top/post/ubuntu_kdump_crash/

基本使用:
```sh
sudo dnf install kdumpctl
sudo systemctl enable kdump.service
```

## 调试无法生成的 vmcore 的一般方法
<!-- ca51101e-7c37-4e03-8434-8b5dbcb0bd21 -->

一共就这两个配置文件:
- /etc/sysconfig/kdump
- /etc/kdump.conf

kdumpctl reload

cat /sys/kernel/kexec_crash_loaded 来检查

还有就需要看看 kdump kernel 启动之后的内核日志。

## 调试为什么 kunpeng 机器没有 kdump

### nvme 的额外参数
```txt
[    8.555004][    T1] systemd[1]: Finished Create List of Static Device Nodes.
[    8.570730][  T149] device-mapper: ioctl: 4.50.0-ioctl (2025-04-28) initialised: dm-devel@lists.linux.dev
[    8.595292][    T1] systemd[1]: Finished Create Static Device Nodes in /dev.
[    8.634538][    T1] systemd[1]: Finished Load Kernel Modules.
[    8.658170][    T1] systemd[1]: Reached target Preparation for Local File Systems.
[    8.685296][    T1] systemd[1]: Reached target Local File Systems.
[    8.718876][    T1] systemd[1]: Starting Apply Kernel Variables...
[    8.739990][    T1] systemd[1]: Started Journal Service.
[    9.038464][  T258] evm: overlay not supported
[    9.913696][  T282] ACPI: bus type drm_connector registered
[    9.953755][  T282] [drm] forcing VGA-1 connector on
[    9.964327][  T282] [drm] Initialized hibmc 1.0.0 for 0000:03:00.0 on minor 0
[   10.013056][  T282] Console: switching to colour frame buffer device 80x30
[   10.061674][  T282] hibmc-drm 0000:03:00.0: [drm] fb0: hibmcdrmfb frame buffer device
[   10.136313][  T282] scsi host0: hisi_sas_v3_hw
[   11.938049][  T282] hisi_sas_v3_hw 0000:74:02.0: neither _PS0 nor _PR0 is defined
[   11.982235][  T282] scsi host1: hisi_sas_v3_hw
[   13.014044][  T282] hisi_sas_v3_hw 0000:74:04.0: neither _PS0 nor _PR0 is defined
[   13.039407][  T282] scsi host2: hisi_sas_v3_hw
[   14.010012][  T282] hisi_sas_v3_hw 0000:b4:02.0: neither _PS0 nor _PR0 is defined
[   14.034308][  T282] scsi host3: hisi_sas_v3_hw
[   15.006013][  T282] hisi_sas_v3_hw 0000:b4:04.0: neither _PS0 nor _PR0 is defined
[   16.054020][  T282] nvme: `4' invalid for parameter `poll_queues'
[   16.094718][  T281] nvme: `4' invalid for parameter `poll_queues'
[   16.190623][  T282] sbsa-gwdt sbsa-gwdt.0: Initialized with 10s timeout @ 100000000 Hz, action=0.
```

但是这只是干扰因素，真实的原因是 nvme 配置的参数 : nvme.poll_queues=4

### 完全不能正常生成
```txt
[  556.195087][ T3839] Call Trace:
[  556.198622][ T3839]  <TASK>
[  556.201775][ T3839]  dump_stack_lvl+0x32/0x50
[  556.206688][ T3839]  panic+0x340/0x360
[  556.210915][ T3839]  ? _printk+0x60/0x80
[  556.215340][ T3839]  sysrq_handle_crash+0x11/0x20
[  556.220644][ T3839]  __handle_sysrq+0x9b/0x190
[  556.225652][ T3839]  write_sysrq_trigger+0x24/0x40
[  556.231049][ T3839]  proc_reg_write+0x55/0xa0
[  556.235960][ T3839]  vfs_write+0xe9/0x3d0
[  556.240482][ T3839]  ? __count_memcg_events+0x4e/0xb0
[  556.246172][ T3839]  ? handle_mm_fault+0x9d/0x370
[  556.251473][ T3839]  ksys_write+0x6b/0xf0
[  556.255992][ T3839]  do_syscall_64+0x3f/0xa0
[  556.260805][ T3839]  entry_SYSCALL_64_after_hwframe+0x78/0xe2
[  556.267276][ T3839] RIP: 0033:0x7fdba44efba0
[  556.272087][ T3839] Code: 73 01 c3 48 8b 0d d0 72 2d 00 f7 d8 64 89 01 48 83 c8 ff c3 66 0f 1f 44 00 00 83 3d 1d d4 2d 00 00 75 10 b8 01 00 00 00 0f 05 <48> 3d 01 f0 ff ff 73 31 c3 48 83 ec 08 e8 7e cc 01 00 48 89 04 24
[  556.293977][ T3839] RSP: 002b:00007ffdbc7072f8 EFLAGS: 00000246 ORIG_RAX: 0000000000000001
[  556.303280][ T3839] RAX: ffffffffffffffda RBX: 0000000000000002 RCX: 00007fdba44efba0
[  556.312093][ T3839] RDX: 0000000000000002 RSI: 00007fdba53b9000 RDI: 0000000000000001
[  556.320904][ T3839] RBP: 00007fdba53b9000 R08: 000000000000000a R09: 00007fdba53b1740
[  556.329717][ T3839] R10: 00007fdba53b1740 R11: 0000000000000246 R12: 00007fdba47c8400
[  556.338526][ T3839] R13: 0000000000000002 R14: 0000000000000001 R15: 0000000000000000
[  556.347338][ T3839]  </TASK>
[  556.350643][ T3839] Kernel Offset: disabled
[  556.369810][ T3839] pstore: backend (erst) writing error (-28)
```

## 为什么生成 vmcore-dmesg-incomplete.txt

这里看到 vmcore-dmesg-incomplete.txt 的容量是空的

但是并不影响，可以继续使用 vmcore ，在其中可以获取到 dmesg 信息:
```txt
'127.0.0.1-2025-12-01-16:03:20':
total 1898944
drwxr-xr-x   2 root root       4096 Dec  1 16:03 .
drwxr-xr-x. 14 root root       4096 Dec  1 16:03 ..
-rw-------   1 root root 1944502512 Dec  1 16:03 vmcore
-rw-r--r--   1 root root          0 Dec  1 16:03 vmcore-dmesg-incomplete.txt
```

## crash 无法使用

看到这个 struct 无法解析的，多数都是 crash 版本不够，无法识别内核的数据结构。

```txt
GNU gdb (GDB) 7.6
Copyright (C) 2013 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
and "show warranty" for details.
This GDB was configured as "x86_64-unknown-linux-gnu"...

WARNING: kernel relocated [628MB]: patching 123399 gdb minimal_symbol values

please wait... (gathering task table data)
crash: invalid structure member offset: task_struct_cpu
       FILE: task.c  LINE: 2874  FUNCTION: add_context()

[/usr/bin/crash] error trace: 557d2d0186e6 => 557d2d00fae8 => 557d2d08c157 => 557d2cf9d817
```

如果是这种错误，那么就的确没有办法了:
```txt
GNU gdb (GDB) 16.2
Copyright (C) 2024 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-pc-linux-gnu".
Type "show configuration" for configuration details.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...

WARNING: failed to init kexec backup region
crash: cannot determine thread return address
crash: invalid kernel virtual address: 0  type: "memory section root table"
```

也就是，如果发现 memory offset 之类的错误，那么就没办法分析了。

哦，原来这个问题是我回答的 : https://unix.stackexchange.com/questions/671800/unable-to-get-kernel-crash-dump-on-kernel-panic

## 也许可以用 kexec 来调试没有生成的情况
kexec -p [/boot/vmlinuz-linux-kdump] --initrd=[/boot/initramfs-linux-kdump.img] --append="root=[root-device] single irqpoll maxcpus=1 reset_devices"
sudo kexec -p /run/current-system/kernel \
        --initrd=/run/current-system/initrd \
        --append="init=$(readlink -f /run/current-system/init) irqpoll maxcpus=1 reset_devices"

也就是配置上完全相同的参数，然后用 kexec 内核启动。

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
