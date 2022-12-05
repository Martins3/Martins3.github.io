## avic
- [ ] https://lwn.net/Articles/680619/
- [ ] https://lwn.net/Articles/895708/

## nested

## svm

### 尝试观察一下时钟是如何实现的?

- `kvm_x86_init_ops` 提供对于 vmcs 的标准访问，和 `kvm_x86_ops` 的关系
  - 后者是: `runtime_ops`

- `kvm_init`
  - `kvm_arch_hardware_setup`
    - `kvm_ops_update`: 更新 `runtime_ops`
  - `kvm_async_pf_init`

- `kvm_emulate_wrmsr`
  - `kvm_complete_insn_gp` : intel 的实现
  - `svm_complete_emulated_msr` : svm 的实现
    - `kvm_complete_insn_gp`
      - `kvm_skip_emulated_instruction` : 和下面的 backtrace 对应起来

跟踪一下:
- `APIC_LVT_TIMER_TSCDEADLINE`


- `kvm_hv_vapic_msr_write` / `kvm_x2apic_msr_write`
  - `kvm_lapic_msr_write`
    - `kvm_lapic_reg_write`
      - `apic_update_lvtt`


实际上，走到这里就问题了
- `emulator_set_msr` ：这是我之前一直好奇的东西
  - `kvm_set_msr`
    - `kvm_set_msr_ignored_check`
    - `__kvm_set_msr`
      - `static_call(kvm_x86_set_msr)(vcpu, &msr);`
        - `svm_set_msr`
          - `kvm_set_msr_common`


check clock event 和 source:
```sh
cat /sys/devices/system/clockevents/broadcast/current_device
cat /sys/devices/system/clockevents/clockevent0/current_device
```


## 的确是使用了 kvm-pit

```sh
ps -elf | grep kvm

1 I root     2488068       2  0  60 -20 -     0 -      12:10 ?        00:00:00 [kvm]
1 S root     2488069       2  0  80   0 -     0 -      12:10 ?        00:00:00 [kvm-nx-lpage-recovery-2488065]
1 S root     2488079       2  0  80   0 -     0 -      12:10 ?        00:00:00 [kvm-pit/2488065]
```

## 一些 backtrace

### `x86_decode_insn`
```txt
@[
    x86_decode_insn+1
    x86_decode_emulated_instruction+54
    x86_emulate_instruction+162
    skip_emulated_instruction+320
    kvm_skip_emulated_instruction+28
    vmx_handle_exit+1838
    kvm_arch_vcpu_ioctl_run+3623
    kvm_vcpu_ioctl+621
    __x64_sys_ioctl+130
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+68
]: 5
@[
    x86_decode_insn+1
    x86_decode_emulated_instruction+54
    x86_emulate_instruction+162
    vmx_handle_exit+369
    kvm_arch_vcpu_ioctl_run+3623
    kvm_vcpu_ioctl+621
    __x64_sys_ioctl+130
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+68
]: 18
@[
    x86_decode_insn+1
    x86_decode_emulated_instruction+54
    x86_emulate_instruction+162
    vmx_handle_exit+1838
    kvm_arch_vcpu_ioctl_run+3623
    kvm_vcpu_ioctl+621
    __x64_sys_ioctl+130
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+68
]: 184
```

### `start_sw_timer`

```txt
@[
    start_sw_timer+1
    restart_apic_timer+110
    handle_fastpath_set_msr_irqoff+220
    kvm_arch_vcpu_ioctl_run+3236
    kvm_vcpu_ioctl+621
    __x64_sys_ioctl+130
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+68
]: 24
@[
    start_sw_timer+1
    kvm_lapic_switch_to_sw_timer+63
    kvm_arch_vcpu_ioctl_run+2628
    kvm_vcpu_ioctl+621
    __x64_sys_ioctl+130
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+68
]: 290
```
