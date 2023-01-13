# 分析下 arch/x86/kvm/vmx/vmx.c 中的内容

## msr

```txt
@[
    vmx_set_msr+1
    kvm_vcpu_reset+1120
    kvm_arch_vcpu_create+604
    kvm_vm_ioctl+2003
    __x64_sys_ioctl+135
    do_syscall_64+56
    entry_SYSCALL_64_after_hwframe+99
]: 6
@[
    vmx_set_msr+1
    __kvm_set_msr+127
    kvm_emulate_wrmsr+82
    vmx_handle_exit+1800
    kvm_arch_vcpu_ioctl_run+3410
    kvm_vcpu_ioctl+625
    __x64_sys_ioctl+135
    do_syscall_64+56
    entry_SYSCALL_64_after_hwframe+99
]: 880
@[
    vmx_set_msr+1
    __kvm_set_msr+127
    kvm_arch_vcpu_ioctl+3148
    kvm_vcpu_ioctl+1204
    __x64_sys_ioctl+135
    do_syscall_64+56
    entry_SYSCALL_64_after_hwframe+99
]: 1692
```

- QEMU 会初始化 msr
   - kvm_init_msrs

```txt
#0  kvm_buf_set_msrs (cpu=cpu@entry=0x555556867920) at ../target/i386/kvm/kvm.c:3166
#1  0x0000555555a6395a in kvm_init_msrs (cpu=<optimized out>) at ../target/i386/kvm/kvm.c:3212
#2  kvm_arch_init_vcpu (cs=cs@entry=0x555556867920) at ../target/i386/kvm/kvm.c:2166
#3  0x0000555555b4f600 in kvm_init_vcpu (cpu=cpu@entry=0x555556867920, errp=0x5555564ad5e0 <error_fatal>) at ../accel/kvm/kvm-all.c:445
#4  0x0000555555b54705 in kvm_vcpu_thread_fn (arg=arg@entry=0x555556867920) at ../accel/kvm/kvm-accel-ops.c:42
#5  0x0000555555cc72f9 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:505
#6  0x00007ffff6888e86 in start_thread () from /nix/store/4nlgxhb09sdr51nc9hdm8az5b08vzkgx-glibc-2.35-163/lib/libc.so.6
#7  0x00007ffff690fc60 in clone3 () from /nix/store/4nlgxhb09sdr51nc9hdm8az5b08vzkgx-glibc-2.35-163/lib/libc.so.6
```

```txt
#0  kvm_put_one_msr (value=<optimized out>, index=<optimized out>, cpu=<optimized out>) at ../target/i386/kvm/kvm.c:2939
#1  kvm_put_msr_feature_control (cpu=0x555556835850) at ../target/i386/kvm/kvm.c:3004
#2  kvm_arch_put_registers (cpu=cpu@entry=0x555556835850, level=level@entry=3) at ../target/i386/kvm/kvm.c:4611
#3  0x0000555555b4cdbe in do_kvm_cpu_synchronize_post_init (cpu=0x555556835850, arg=...) at ../accel/kvm/kvm-all.c:2737
#4  0x0000555555824908 in process_queued_cpu_work (cpu=0x555556835850) at ../cpus-common.c:351
#5  0x0000555555b54730 in kvm_vcpu_thread_fn (arg=arg@entry=0x555556835850) at ../accel/kvm/kvm-accel-ops.c:56
#6  0x0000555555cc72f9 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:505
#7  0x00007ffff6888e86 in start_thread () from /nix/store/4nlgxhb09sdr51nc9hdm8az5b08vzkgx-glibc-2.35-163/lib/libc.so.6
#8  0x00007ffff690fc60 in clone3 () from /nix/store/4nlgxhb09sdr51nc9hdm8az5b08vzkgx-glibc-2.35-163/lib/libc.so.6
```

- Guest 读写 msr 的时候:

- kvm_emulate_wrmsr
  - kvm_set_msr_with_filter
    - kvm_msr_allowed :
    - kvm_set_msr_ignored_check
      - `__kvm_set_msr`
        - vmx_set_msr
        - svm_set_msr

```txt
Numeric Error Handling (FERR# and IGNNE#)
The x86 processors that have integrated floating-point units provide support for DOS compatible error-handling. These processors have a numeric exception (NE) control bit in Control Register 0 that allows the programmer to select the error-handling scenario to be used by the processor when a floating-point error is detected. The error handling options are:

Native error handling — this method of numeric error handling involves the processor reporting the error via processor exception (16d), which calls the error handling code.

DOS-compatible error handling — this method ensures that numeric error code designed for IBM PC-AT compatible systems (ISA machines) running DOS will work correctly with later processors. ...
```

```txt
15.30.2 IGNNE MSR (C001_0115h)
The read/write IGNNE MSR is used to set the state of the processor-internal IGNNE signal directly.
This is only useful if IGNNE emulation has been enabled in the HW_CR MSR (and thus the external
signal is being ignored). Bit 0 specifies the current value of IGNNE; all other bits are MBZ.
```
什么叫做 input signal 啊？
