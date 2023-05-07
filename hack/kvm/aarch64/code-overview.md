```txt
🧀  tokei -f  .
===============================================================================================================================
 Language                                                            Files        Lines         Code     Comments       Blanks
===============================================================================================================================
 GNU Style Assembly                                                      7         1088          661          241          186
-------------------------------------------------------------------------------------------------------------------------------
 ./hyp/nvhe/hyp.lds.S                                                                29           15           11            3
 ./hyp/nvhe/cache.S                                                                  25           17            4            4
 ./hyp/nvhe/hyp-init.S                                                              271          159           61           51
 ./hyp/fpsimd.S                                                                      27           16            5            6
 ./hyp/nvhe/host.S                                                                  265          160           65           40
 ./hyp/entry.S                                                                      215          108           65           42
 ./hyp/hyp-entry.S                                                                  256          186           30           40
-------------------------------------------------------------------------------------------------------------------------------
 C                                                                      63        34744        23137         5835         5772
-------------------------------------------------------------------------------------------------------------------------------
 ./vgic/vgic-debug.c                                                                280          218           19           43
 ./vgic/vgic-mmio-v2.c                                                              561          433           42           86
 ./vgic/vgic-irqfd.c                                                                155          104           33           18
 ./vgic/vgic-v3.c                                                                   763          524          105          134
 ./vgic/vgic-v4.c                                                                   512          279          155           78
 ./vgic/vgic-v2.c                                                                   486          334           64           88
 ./vgic/vgic-kvm-device.c                                                           695          538           62           95
 ./vgic/vgic-init.c                                                                 602          347          162           93
 ./inject_fault.c                                                                   205          105           71           29
 ./vgic/vgic-mmio.c                                                                1120          785          136          199
 ./vgic-sys-reg-v3.c                                                                366          272           17           77
 ./stacktrace.c                                                                     245          142           70           33
 ./vmid.c                                                                           196          110           50           36
 ./vgic/vgic-mmio-v3.c                                                             1168          850          146          172
 ./fpsimd.c                                                                         215          107           76           32
 ./trng.c                                                                            85           65            4           16
 ./psci.c                                                                           458          301           99           58
 ./vgic/vgic.c                                                                     1061          599          284          178
 ./va_layout.c                                                                      298          180           69           49
 ./pmu.c                                                                            211          138           45           28
 ./handle_exit.c                                                                    376          231           92           53
 ./hypercalls.c                                                                     481          368           61           52
 ./mmu.c                                                                           1928         1160          489          279
 ./hyp/vhe/sysreg-sr.c                                                              114           53           43           18
 ./hyp/vhe/debug-sr.c                                                                26           15            5            6
 ./hyp/vhe/timer-sr.c                                                                12            5            5            2
 ./hyp/vhe/tlb.c                                                                    163           81           54           28
 ./hyp/nvhe/early_alloc.c                                                            59           40            5           14
 ./reset.c                                                                          397          236           97           64
 ./hyp/vhe/switch.c                                                                 245          142           51           52
 ./arm.c                                                                           2328         1517          418          393
 ./hyp/nvhe/sysreg-sr.c                                                              35           21            8            6
 ./hyp/nvhe/psci-relay.c                                                            304          216           34           54
 ./hyp/nvhe/stacktrace.c                                                            158           91           46           21
 ./hyp/nvhe/gen-hyprel.c                                                            456          301          100           55
 ./hyp/nvhe/mm.c                                                                    374          247           52           75
 ./sys_regs.c                                                                      3100         2187          454          459
 ./hyp/nvhe/mem_protect.c                                                          1230          927           69          234
 ./guest.c                                                                         1104          782          113          209
 ./hyp/nvhe/setup.c                                                                 360          250           45           65
 ./hyp/nvhe/timer-sr.c                                                               48           23           18            7
 ./hyp/nvhe/debug-sr.c                                                              115           70           25           20
 ./hyp/nvhe/hyp-smp.c                                                                40           21           11            8
 ./hyp/nvhe/list_debug.c                                                             54           38            6           10
 ./hyp/nvhe/tlb.c                                                                   161           72           62           27
 ./vgic/vgic-its.c                                                                 2885         1941          437          507
 ./hyp/nvhe/sys_regs.c                                                              522          318          121           83
 ./hyp/nvhe/hyp-main.c                                                              406          287           32           87
 ./arch_timer.c                                                                    1388          998          132          258
 ./hyp/aarch32.c                                                                    140           76           42           22
 ./hyp/hyp-constants.c                                                               13           10            1            2
 ./hyp/nvhe/page_alloc.c                                                            247          140           64           43
 ./hyp/vgic-v2-cpuif-proxy.c                                                         89           53           22           14
 ./hyp/exception.c                                                                  351          191          105           55
 ./pkvm.c                                                                           215          147           28           40
 ./hyp/nvhe/switch.c                                                                370          221           84           65
 ./mmio.c                                                                           196          140           27           29
 ./hyp/pgtable.c                                                                   1269          919          102          248
 ./pvtime.c                                                                         133          101            7           25
 ./hyp/vgic-v3-sr.c                                                                1143          855          110          178
 ./hyp/nvhe/pkvm.c                                                                  621          395          112          114
 ./debug.c                                                                          344          147          147           50
 ./pmu-emul.c                                                                      1062          673          190          199
-------------------------------------------------------------------------------------------------------------------------------
 C Header                                                               21         3066         2105          445          516
-------------------------------------------------------------------------------------------------------------------------------
 ./vgic/trace.h                                                                      38           27            2            9
 ./vgic/vgic-mmio.h                                                                 230          159           24           47
 ./vgic/vgic.h                                                                      335          254           32           49
 ./trace_handle_exit.h                                                              219          170            6           43
 ./trace_arm.h                                                                      312          243            8           61
 ./sys_regs.h                                                                       227          169           20           38
 ./trace.h                                                                            8            5            1            2
 ./hyp/include/nvhe/early_alloc.h                                                    14            9            1            4
 ./hyp/include/nvhe/mm.h                                                             31           24            1            6
 ./hyp/include/nvhe/memory.h                                                         75           55            5           15
 ./hyp/include/nvhe/gfp.h                                                            34           20            8            6
 ./hyp/include/nvhe/trap_handler.h                                                   20            8            7            5
 ./hyp/include/hyp/debug-sr.h                                                       168          138            5           25
 ./hyp/include/nvhe/pkvm.h                                                           68           34           19           15
 ./hyp/include/nvhe/fixed_config.h                                                  205           93           94           18
 ./hyp/include/hyp/sysreg-sr.h                                                      219          151           34           34
 ./hyp/include/nvhe/mem_protect.h                                                    90           63           15           12
 ./hyp/include/hyp/fault.h                                                           75           37           27           11
 ./hyp/include/hyp/adjust_pc.h                                                       53           27           17            9
 ./hyp/include/nvhe/spinlock.h                                                      125           80           29           16
 ./hyp/include/hyp/switch.h                                                         520          339           90           91
-------------------------------------------------------------------------------------------------------------------------------
 Makefile                                                                4          174           84           58           32
-------------------------------------------------------------------------------------------------------------------------------
 ./hyp/vhe/Makefile                                                                  11            5            4            2
 ./hyp/nvhe/Makefile                                                                112           48           46           18
 ./hyp/Makefile                                                                      10            4            4            2
 ./Makefile                                                                          41           27            4           10
===============================================================================================================================
 Total                                                                  95        39072        25987         6579         6506
===============================================================================================================================
```

## 大致分析下
- kvm/vgic/vgic-its.c
- arch_timer.c
- hyp/vgic-v3-sr.c
