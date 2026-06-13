```txt
 ./arch_timer.c                                                                    1388          998          132          258
 ./arm.c                                                                           2328         1517          418          393
 ./debug.c                                                                          344          147          147           50
 ./fpsimd.c                                                                         215          107           76           32
 ./guest.c                                                                         1104          782          113          209
 ./handle_exit.c                                                                    376          231           92           53
 ./hyp/aarch32.c                                                                    140           76           42           22
 ./hyp/exception.c                                                                  351          191          105           55
 ./hyp/hyp-constants.c                                                               13           10            1            2
 ./hyp/nvhe/debug-sr.c                                                              115           70           25           20
 ./hyp/nvhe/early_alloc.c                                                            59           40            5           14
 ./hyp/nvhe/gen-hyprel.c                                                            456          301          100           55
 ./hyp/nvhe/hyp-main.c                                                              406          287           32           87
 ./hyp/nvhe/hyp-smp.c                                                                40           21           11            8
 ./hyp/nvhe/list_debug.c                                                             54           38            6           10
 ./hyp/nvhe/mem_protect.c                                                          1230          927           69          234
 ./hyp/nvhe/mm.c                                                                    374          247           52           75
 ./hyp/nvhe/page_alloc.c                                                            247          140           64           43
 ./hyp/nvhe/pkvm.c                                                                  621          395          112          114
 ./hyp/nvhe/psci-relay.c                                                            304          216           34           54
 ./hyp/nvhe/setup.c                                                                 360          250           45           65
 ./hyp/nvhe/stacktrace.c                                                            158           91           46           21
 ./hyp/nvhe/switch.c                                                                370          221           84           65
 ./hyp/nvhe/sys_regs.c                                                              522          318          121           83
 ./hyp/nvhe/sysreg-sr.c                                                              35           21            8            6
 ./hyp/nvhe/timer-sr.c                                                               48           23           18            7
 ./hyp/nvhe/tlb.c                                                                   161           72           62           27
 ./hyp/pgtable.c                                                                   1269          919          102          248
 ./hyp/vgic-v2-cpuif-proxy.c                                                         89           53           22           14
 ./hyp/vgic-v3-sr.c                                                                1143          855          110          178
 ./hyp/vhe/debug-sr.c                                                                26           15            5            6
 ./hyp/vhe/switch.c                                                                 245          142           51           52
 ./hyp/vhe/sysreg-sr.c                                                              114           53           43           18
 ./hyp/vhe/timer-sr.c                                                                12            5            5            2
 ./hyp/vhe/tlb.c                                                                    163           81           54           28
 ./hypercalls.c                                                                     481          368           61           52
 ./inject_fault.c                                                                   205          105           71           29
 ./mmio.c                                                                           196          140           27           29
 ./mmu.c                                                                           1928         1160          489          279
 ./pkvm.c                                                                           215          147           28           40
 ./pmu-emul.c                                                                      1062          673          190          199
 ./pmu.c                                                                            211          138           45           28
 ./psci.c                                                                           458          301           99           58
 ./pvtime.c                                                                         133          101            7           25
 ./reset.c                                                                          397          236           97           64
 ./stacktrace.c                                                                     245          142           70           33
 ./sys_regs.c                                                                      3100         2187          454          459
 ./trng.c                                                                            85           65            4           16
 ./va_layout.c                                                                      298          180           69           49
 ./vgic-sys-reg-v3.c                                                                366          272           17           77
 ./vgic/vgic-debug.c                                                                280          218           19           43
 ./vgic/vgic-init.c                                                                 602          347          162           93
 ./vgic/vgic-irqfd.c                                                                155          104           33           18
 ./vgic/vgic-its.c                                                                 2885         1941          437          507
 ./vgic/vgic-kvm-device.c                                                           695          538           62           95
 ./vgic/vgic-mmio-v2.c                                                              561          433           42           86
 ./vgic/vgic-mmio-v3.c                                                             1168          850          146          172
 ./vgic/vgic-mmio.c                                                                1120          785          136          199
 ./vgic/vgic-v2.c                                                                   486          334           64           88
 ./vgic/vgic-v3.c                                                                   763          524          105          134
 ./vgic/vgic-v4.c                                                                   512          279          155           78
 ./vgic/vgic.c                                                                     1061          599          284          178
 ./vmid.c                                                                           196          110           50           36
```

## 大致分析下
- kvm/vgic/vgic-its.c
- arch_timer.c
- hyp/vgic-v3-sr.c

## arm 的 idle 状态的时候的 kvm_stat 的输出和 x86 的差别很大

```txt
kvm statistics - summary

 Event                                         Total %Total CurAvg/s
 kvm_get_timer_map                            389253    9.7      165
 kvm_arm_set_dreg32                           693512   17.3      121
 vgic_update_irq_pending                      276914    6.9      111
 kvm_timer_update_irq                         276914    6.9      111
 kvm_timer_save_state                         224920    5.6      107
 kvm_timer_restore_state                      224920    5.6      107
 kvm_arm_setup_debug                          346756    8.7       61
 kvm_arm_clear_debug                          346756    8.7       61
 kvm_entry                                    346756    8.7       61
 kvm_exit                                     346756    8.7       61
 kvm_wfx_arm64                                155013    3.9       54
 kvm_vcpu_wakeup                              155013    3.9       54
 kvm_handle_sys_reg                            53882    1.3        2
 kvm_sys_access                                53882    1.3        2
 kvm_halt_poll_ns                              40681    1.0        1
 kvm_guest_fault                               38630    1.0        0
 kvm_mmio                                      37487    0.9        0
 kvm_unmap_hva_range                             442    0.0        0
 kvm_set_irq                                      50    0.0        0
 kvm_hvc_arm64                                    13    0.0        0
 kvm_userspace_exit                                3    0.0        0
 kvm_set_spte_hva                                  3    0.0        0
 Total                                       4008556            1078
```

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
