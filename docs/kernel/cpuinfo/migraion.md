# migration 的 cpu flags

## 是如何发送 cpu flags 的

## 是如何检查 cpu flags 的

各种 CPU 的 load 和 save 主要内容在 target/i386/machine.c 中的，

- cpuflags 的检查也不是在启动过程中的?
- cpuflags 不满足在哪里报错，根本就没有哇，兄弟们

## kvm_arch_init_vcpu 的调用路径上存在检查吗？

倒是这个多出来很多，但是这只是 kvm 的检查
```c
int kvm_check_extension(KVMState *s, unsigned int extension)
{
    int ret;

    ret = kvm_ioctl(s, KVM_CHECK_EXTENSION, extension);
    if (ret < 0) {
        ret = 0;
    }

    return ret;
}
```

我认为问题的关键在于 set_cpuid 的时候没有

- `kvm_vcpu_ioctl_set_cpuid2`
  - `kvm_set_cpuid`
    - `__kvm_update_cpuid_runtime`
    - kvm_cpuid_check_equal : 动态修改 cpu model 是不允许的
    - `kvm_check_cpuid` : 难道在这里检查吗？
    - kvm_vcpu_after_set_cpuid
      - static_call(kvm_x86_vcpu_after_set_cpuid)(vcpu);
        - vmx_vcpu_after_set_cpuid
          - boot_cpu_has(X86_FEATURE_RTM)

## 初始化
- 在 kvm_arch_init_vcpu 中
1. 初始化 cpuid_data
2. 反复的调用 cpu_x86_cpuid 通过 CPUX86State 来初始化 cpuid_data 的，
3. kvm_vcpu_ioctl(cs, KVM_SET_CPUID2, &cpuid_data);

调用顺序，首先走这里:
```txt
#0  x86_cpu_filter_features (cpu=cpu@entry=0x555556a93420, verbose=true) at ../target/i386/cpu.c:6457
#1  0x0000555555b35589 in x86_cpu_realizefn (dev=0x555556a93420, errp=0x7fffffff9910) at ../target/i386/cpu.c:6600
#2  0x0000555555ca291b in device_set_realized (obj=<optimized out>, value=<optimized out>, errp=0x7fffffff9990) at ../hw/core/qdev.c:510
#3  0x0000555555ca6696 in property_set_bool (obj=0x555556a93420, v=<optimized out>, name=<optimized out>, opaque=0x5555567ca560, errp=0x7fffffff9990) at ../qom/object.c:2285
#4  0x0000555555ca9654 in object_property_set (obj=obj@entry=0x555556a93420, name=name@entry=0x555555f57223 "realized", v=v@entry=0x555556a9b840, errp=0x7fffffff9990, errp@entry=0x55555672f9b8 <error_fatal>) at ../qom/object.c:1420
#5  0x0000555555cac9f0 in object_property_set_qobject (obj=obj@entry=0x555556a93420, name=name@entry=0x555555f57223 "realized", value=value@entry=0x5555567ca790, errp=errp@entry=0x55555672f9b8 <error_fatal>) at ../qom/qom-qobject.c:28
#6  0x0000555555ca9c95 in object_property_set_bool (obj=0x555556a93420, name=name@entry=0x555555f57223 "realized", value=value@entry=true, errp=errp@entry=0x55555672f9b8 <error_fatal>) at ../qom/object.c:1489
#7  0x0000555555ca329e in qdev_realize (dev=<optimized out>, bus=bus@entry=0x0, errp=errp@entry=0x55555672f9b8 <error_fatal>) at ../hw/core/qdev.c:292
#8  0x0000555555afaee1 in x86_cpu_new (x86ms=<optimized out>, apic_id=0, errp=0x55555672f9b8 <error_fatal>) at ../hw/i386/x86.c:105
#9  0x0000555555afafda in x86_cpus_init (x86ms=x86ms@entry=0x555556a18800, default_cpu_version=<optimized out>) at ../hw/i386/x86.c:151
#10 0x0000555555b01765 in pc_init1 (machine=0x555556a18800, pci_type=0x555555f3008e "i440FX", host_type=0x555555f300ad "i440FX-pcihost") at ../hw/i386/pc_piix.c:175
#11 0x00005555558e794c in machine_run_board_init (machine=0x555556a18800, mem_path=<optimized out>, errp=<optimized out>) at ../hw/core/machine.c:1408
#12 0x0000555555a5a906 in qemu_init_board () at ../softmmu/vl.c:2513
#13 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2609
#14 0x0000555555a5e47d in qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2604
#15 qemu_init (argc=<optimized out>, argv=<optimized out>) at ../softmmu/vl.c:3612
#16 0x0000555555864919 in main (argc=<optimized out>, argv=<optimized out>) at ../softmmu/main.c:47
```

然后走到这里:
```txt
#0  kvm_arch_init_vcpu (cs=cs@entry=0x555556a93420) at ../target/i386/kvm/kvm.c:1749
#1  0x0000555555c91ef0 in kvm_init_vcpu (cpu=cpu@entry=0x555556a93420, errp=0x55555672f9b8 <error_fatal>) at ../accel/kvm/kvm-all.c:447
#2  0x0000555555c972a5 in kvm_vcpu_thread_fn (arg=arg@entry=0x555556a93420) at ../accel/kvm/kvm-accel-ops.c:42
#3  0x0000555555e18f39 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:541
#4  0x00007ffff6688e86 in start_thread () from /nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/libc.so.6
#5  0x00007ffff670fd70 in clone3 () from /nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/libc.so.6
```
整体逻辑，-cpu 来说明需要那些，不需要那些，但是需要和 kernel 做一次校验，
如果没有，就警告，其实进而 Guest 使用的 flags 也会变少的。

## 所以，这是一个 bug 吗？
