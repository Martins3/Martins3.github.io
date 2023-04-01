# proc
- [ ] clear this document and pick useful info from plka/syn

- [ ] 内核中间只有 /proc/sys 的文档 https://www.kernel.org/doc/html/latest/admin-guide/sysctl/index.html

https://www.kernel.org/doc/html/latest/filesystems/proc.html

## /proc/devices
- fs/proc/devices.c
  - `chrdev_show`
  - `blkdev_show`

## /proc/kcore
2. dynamic kernel image : 128T
```
➜  .SpaceVim.d git:(master) ✗ l /proc/kcore
.r-------- root root 128 TB Tue Mar 17 18:47:48 2020   kcore
```

## sysrq-trigger

### b : 直接 reboot

这个不会清理 cache 的，最后调用到这里:
```c
struct machine_ops machine_ops __ro_after_init = {
	.power_off = native_machine_power_off,
	.shutdown = native_machine_shutdown,
	.emergency_restart = native_machine_emergency_restart,
	.restart = native_machine_restart,
	.halt = native_machine_halt,
#ifdef CONFIG_KEXEC_CORE
	.crash_shutdown = native_machine_crash_shutdown,
#endif
};
```
如果是 `shutdown now` 会存在这个调用
```txt
#0  native_machine_power_off () at arch/x86/kernel/reboot.c:737
#1  0xffffffff8117884e in __do_sys_reboot (magic1=-18751827, magic2=<optimized out>, cmd=1126301404, arg=0x7fcbe9d289e0) at kernel/reboot.c:753
#2  0xffffffff8227db9f in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000017f58) at arch/x86/entry/common.c:50
#3  do_syscall_64 (regs=0xffffc90000017f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#4  0xffffffff824000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```
