rg "^\W+trace_"
```txt
arch_timer.c
190:    trace_kvm_get_timer_map(vcpu->vcpu_id, map);
348:    trace_kvm_timer_hrtimer_expire(ctx);
453:    trace_kvm_timer_update_irq(vcpu->vcpu_id, timer_irq(timer_ctx),
470:    trace_kvm_timer_emulate(ctx, should_fire);
566:    trace_kvm_timer_save_state(ctx);
655:    trace_kvm_timer_restore_state(ctx);

mmio.c
136:            trace_kvm_mmio(KVM_TRACE_MMIO_READ, len, run->mmio.phys_addr,
169:            trace_kvm_mmio_nisv(*vcpu_pc(vcpu), kvm_vcpu_get_esr(vcpu),
201:            trace_kvm_mmio(KVM_TRACE_MMIO_WRITE, len, fault_ipa, &data);
207:            trace_kvm_mmio(KVM_TRACE_MMIO_READ_UNSATISFIED, len,

arm.c
1221:           trace_kvm_entry(*vcpu_pc(vcpu));
1280:           trace_kvm_exit(ret, kvm_vcpu_trap_get_class(vcpu), *vcpu_pc(vcpu));
1380:   trace_kvm_irq_line(irq_type, vcpu_id, irq_num, irq_level->level);

mmu.c
1773:   trace_kvm_access_fault(fault_ipa);
1847:   trace_kvm_guest_fault(*vcpu_pc(vcpu), kvm_vcpu_get_esr(vcpu),
2294:           trace_kvm_set_way_flush(*vcpu_pc(vcpu),
2317:   trace_kvm_toggle_cache(*vcpu_pc(vcpu), was_enabled, now_enabled);

sys_regs.c
4309:   trace_kvm_sys_access(*vcpu_pc(vcpu), params, r);
4817:   trace_kvm_handle_sys_reg(esr);

guest.c
921:    trace_kvm_set_guest_debug(vcpu, dbg->control);

emulate-nested.c
2587:   trace_kvm_forward_sysreg_trap(vcpu, sysreg, is_read);
2696:   trace_kvm_nested_eret(vcpu, elr, spsr);
2712:   trace_kvm_inject_nested_exception(vcpu, esr_el2, type);

handle_exit.c
40:     trace_kvm_hvc_arm64(*vcpu_pc(vcpu), vcpu_get_reg(vcpu, 0),
139:            trace_kvm_wfx_arm64(*vcpu_pc(vcpu), true);
142:            trace_kvm_wfx_arm64(*vcpu_pc(vcpu), false);

vgic/vgic.c
446:    trace_vgic_update_irq_pending(vcpu ? vcpu->vcpu_idx : 0, intid, level);
```

## aarch64 kvm_stat 观测
<!-- 9b686a1b-ba34-404b-b639-00ea6b3bed3b -->

在 6.16 的环境中测试，其结果为:
```txt
kvm_get_timer_map                           1032963   18.3     9838
kvm_timer_update_irq                         697115   12.3     6620
vgic_update_irq_pending                      697115   12.3     6620
kvm_timer_save_state                         676580   12.0     6467
kvm_timer_restore_state                      676580   12.0     6467
kvm_exit                                     477754    8.4     3691
  kvm_exit(WFx)                              413413   86.5     3228
  kvm_exit(SYS64)                             33750    7.1      234
  kvm_exit(UNKNOWN)                           28805    6.0      212
  kvm_exit(DABT_LOW)                           1755    0.4       16
  kvm_exit(HVC64)                                18    0.0        0
  kvm_exit(IABT_LOW)                              1    0.0        0
kvm_entry                                    477754    8.4     3690
kvm_wfx_arm64                                413414    7.3     3228
kvm_vcpu_wakeup                              338921    6.0     3228
kvm_halt_poll_ns                              92889    1.6      613
kvm_sys_access                                33750    0.6      234
kvm_handle_sys_reg                            33750    0.6      234
kvm_mmio                                       1875    0.0       16
kvm_guest_fault                                1756    0.0       16
kvm_userspace_exit                             1484    0.0       15
  kvm_userspace_exit(MMIO)                     1484  100.0       15
kvm_unmap_hva_range                             366    0.0        2
kvm_set_irq                                      46    0.0        0
kvm_hvc_arm64                                    18    0.0        0
Total                                       5654130           50980
```

但是在 4.19 的环境测试发现结果为这个:
```txt
kvm statistics - summary
 Event                                         Total %Total CurAvg/s
 kvm_arm_set_dreg32                           476260   16.5     8838
 kvm_get_timer_map                            337009   11.7     6392
 kvm_exit                                     238130    8.3     4419
   kvm_exit(UNKNOWN)                          124281   52.2     2374
   kvm_exit(WFx)                              112549   47.3     2028
   kvm_exit(SYS64)                               501    0.2       12
   kvm_exit(DABT_LOW)                            723    0.3        6
   kvm_exit(IABT_LOW)                             63    0.0        0
   kvm_exit(HVC64)                                 5    0.0        0
 kvm_entry                                    238130    8.3     4419
 kvm_arm_clear_debug                          238130    8.3     4419
 kvm_arm_setup_debug                          238130    8.3     4419
 kvm_timer_update_irq                         227856    7.9     4362
 vgic_update_irq_pending                      227856    7.9     4362
 kvm_timer_restore_state                      219878    7.6     4079
 kvm_timer_save_state                         219878    7.6     4079
 kvm_wfx_arm64                                112550    3.9     2028
 kvm_vcpu_wakeup                              109141    3.8     2027
 kvm_halt_poll_ns                                806    0.0       21
 kvm_sys_access                                  501    0.0       12
 kvm_handle_sys_reg                              501    0.0       12
 kvm_guest_fault                                 786    0.0        6
 kvm_mmio                                        443    0.0        6
 kvm_userspace_exit                              264    0.0        5
   kvm_userspace_exit(MMIO)                      264  100.0        5
 kvm_hvc_arm64                                     5    0.0        0
 kvm_set_irq                                       4    0.0        0
 Total                                       2886258           53903
```

也就是多出来了很多:
```txt
kvm_arm_set_dreg32                           476260   16.5     8838
kvm_arm_clear_debug                          238130    8.3     4419
kvm_arm_setup_debug                          238130    8.3     4419
```
高版本只是把对应的 tracepoint 给移除掉了而已，具体的代码在
arch/arm64/kvm/debug.c

每次上下文切换都是有 debug 寄存器的恢复，所以可以 trace 到特别多的东西。

解决一下这些问题:

1. kvm_exit 的 reason 都是意思?
2. 既然都是硬件发送中断了，为什么还有这么多时钟中断相关的调用

kvm_get_timer_map                           1032963   18.3     9838
kvm_timer_update_irq                         697115   12.3     6620
vgic_update_irq_pending                      697115   12.3     6620
kvm_timer_save_state                         676580   12.0     6467
kvm_timer_restore_state                      676580   12.0     6467

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
