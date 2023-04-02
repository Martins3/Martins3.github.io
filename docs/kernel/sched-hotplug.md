# 分析 CPU hotplug

关键参考

<p align="center">
  <img src="https://www.dingmos.com/usr/uploads/2022/12/2930619187.png" alt="drawing" align="center"/>
</p>
<p align="center">
[Linux 内核 | CPU 热插拔（Hotplug）](https://www.dingmos.com/index.php/archives/117/)
</p>


通过 /sys/devices/system/cpu/hotplug/states 来查看所有注册的回调函数
## 主要文件
- kernel/cpu.c

## 关键函数分析

```txt
#0  _cpu_down (cpu=cpu@entry=1, tasks_frozen=tasks_frozen@entry=0, target=target@entry=CPUHP_OFFLINE) at kernel/cpu.c:1154
#1  0xffffffff81144914 in cpu_down_maps_locked (target=CPUHP_OFFLINE, cpu=1) at kernel/cpu.c:1228
#2  cpu_down (target=CPUHP_OFFLINE, cpu=1) at kernel/cpu.c:1236
#3  cpu_device_down (dev=<optimized out>) at kernel/cpu.c:1253
#4  0xffffffff81b3b455 in device_offline (dev=0xffff88833365ba48) at drivers/base/core.c:4088
#5  device_offline (dev=0xffff88833365ba48) at drivers/base/core.c:4072
#6  0xffffffff81b3b585 in online_store (dev=0xffff88833365ba48, attr=<optimized out>, buf=<optimized out>, count=2) at drivers/base/core.c:2651
#7  0xffffffff814d312c in kernfs_fop_write_iter (iocb=0xffffc90001ff3ea0, iter=<optimized out>) at fs/kernfs/file.c:334
#8  0xffffffff81423055 in call_write_iter (iter=0xffffc90001ff3e78, kio=0xffffc90001ff3ea0, file=0xffff88810dc17500) at ./include/linux/fs.h:1851
#9  new_sync_write (ppos=0xffffc90001ff3f08, len=2, buf=0x55b7744652a0 "0\n", filp=0xffff88810dc17500) at fs/read_write.c:491
#10 vfs_write (file=file@entry=0xffff88810dc17500, buf=buf@entry=0x55b7744652a0 "0\n", count=count@entry=2, pos=pos@entry=0xffffc90001ff3f08) at fs/read_write.c:584
#11 0xffffffff814234c3 in ksys_write (fd=<optimized out>, buf=0x55b7744652a0 "0\n", count=2) at fs/read_write.c:637
#12 0xffffffff8227db9f in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001ff3f58) at arch/x86/entry/common.c:50
#13 do_syscall_64 (regs=0xffffc90001ff3f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#14 0xffffffff824000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```
- `_cpu_down`
  - cpuhp_kick_ap_work
    - cpuhp_kick_ap
      - 目标函数运行: cpuhp_thread_fun
  - cpuhp_down_callbacks
    - cpuhp_invoke_callback_range

## 问题
### unplug 一个 CPU，然后热迁移，QEMU 如何保证 CPU 的数量

### kvm 和 QEMU 分别需要何种配合
这都是随便搜搜得到，估计这个资料很多吧:
- https://wiki.qemu.org/Features/CPUHotplug
- https://www.qemu.org/docs/master/system/cpu-hotplug.html
- http://events17.linuxfoundation.org/sites/events/files/slides/CPU%20Hot-plug%20support%20in%20QEMU.pdf

### kernel/code/sched-hotplug.sh 执行之后，在 htop 中还是可以看到，只是标志为 offline，lscpu 就真的看不到了

### 为什么需要函数 `cpuhp_threads_init`
online 的时候，会存在调用:
```txt
#0  cpuhp_thread_fun (cpu=1) at kernel/cpu.c:772
#1  0xffffffff8117974f in smpboot_thread_fn (data=0xffff88810027e140) at kernel/smpboot.c:164
#2  0xffffffff8116e96c in kthread (_create=0xffff88810082f840) at kernel/kthread.c:376
#3  0xffffffff810029bc in ret_from_fork () at arch/x86/entry/entry_64.S:308
#4  0x0000000000000000 in ?? ()
```

### 如果当时 CPU 很忙，被要求 offline

```c
static struct syscore_ops kvm_syscore_ops = {
	.suspend	= kvm_suspend,
	.resume		= kvm_resume,
};
```
