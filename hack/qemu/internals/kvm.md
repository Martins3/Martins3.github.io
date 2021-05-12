# KVM Related Stuff in Qemu
> 感觉如果真的想要搞清楚这些东西，首先看懂 kvmtool 也是不错的

> 其实和 kvm 相关的代码并不是很多的。

```
#0  kvm_cpu_exec (cpu=cpu@entry=0x555556f81750) at ../accel/kvm/kvm-all.c:2429
#1  0x0000555555b771a5 in kvm_vcpu_thread_fn (arg=arg@entry=0x555556f81750) at ../accel/kvm/kvm-accel-ops.c:49
#2  0x0000555555d248b3 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:521
#3  0x00007ffff6096609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#4  0x00007ffff5fbb293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
```

