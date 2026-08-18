# 简单分析下 GPU 驱动

说不定之后要分析 virtio-gpu 的。

首先，即使是集成显卡，也是有 GPU 的:

```txt
[114174.876779] i915 0000:00:02.0: [drm] GPU HANG: ecode 12:1:859ffffb, in thunderbird [19356]
[114174.877344] i915 0000:00:02.0: [drm] Resetting chip for stopped heartbeat on rcs0
[114174.978503] i915 0000:00:02.0: [drm] thunderbird[19356] context reset due to GPU hang
[114174.978542] i915 0000:00:02.0: [drm] GuC firmware i915/tgl_guc_70.1.1.bin version 70.1
[114174.978544] i915 0000:00:02.0: [drm] HuC firmware i915/tgl_huc_7.9.3.bin version 7.9
[114174.982370] i915 0000:00:02.0: [drm] HuC authenticated
[114174.982603] i915 0000:00:02.0: [drm] GuC submission enabled
[114174.982604] i915 0000:00:02.0: [drm] GuC SLPC enabled
```

内核启动的时候的样子：

```txt
[    2.183239] systemd[1]: Starting Load Kernel Module drm...
[    2.201508] ACPI: bus type drm_connector registered
[    2.809626] i915 0000:00:02.0: vgaarb: deactivate vga console
[    2.809655] i915 0000:00:02.0: [drm] Using Transparent Hugepages
[    2.810215] i915 0000:00:02.0: vgaarb: changed VGA decodes: olddecodes=io+mem,decodes=io+mem:owns=io+mem
[    2.811237] i915 0000:00:02.0: [drm] Finished loading DMC firmware i915/adls_dmc_ver2_01.bin (v2.1)
[    2.824596] i915 0000:00:02.0: [drm] GuC firmware i915/tgl_guc_70.1.1.bin version 70.1
[    2.824598] i915 0000:00:02.0: [drm] HuC firmware i915/tgl_huc_7.9.3.bin version 7.9
[    2.827208] i915 0000:00:02.0: [drm] HuC authenticated
[    2.827470] i915 0000:00:02.0: [drm] GuC submission enabled
[    2.827471] i915 0000:00:02.0: [drm] GuC SLPC enabled
[    2.827881] i915 0000:00:02.0: [drm] GuC RC: enabled
[    2.833849] iwlwifi 0000:00:14.3: Detected Intel(R) Wi-Fi 6 AX201 160MHz, REV=0x430
[    2.833873] thermal thermal_zone1: failed to read out thermal zone (-61)
[    2.847202] [drm] Initialized i915 1.6.0 20201103 for 0000:00:02.0 on minor 0
[    2.847666] ACPI: video: Video Device [GFX0] (multi-head: yes  rom: no  post: no)
[    2.847903] input: Video Bus as /devices/LNXSYSTM:00/LNXSYBUS:00/PNP0A08:00/LNXVIDEO:00/input/input8
[    2.847950] snd_hda_intel 0000:00:1f.3: bound 0000:00:02.0 (ops i915_audio_component_bind_ops [i915])
[    2.877281] fbcon: i915drmfb (fb0) is primary device
[    2.919750] Console: switching to colour frame buffer device 480x135
[    2.938955] i915 0000:00:02.0: [drm] fb0: i915drmfb frame buffer device
```

## drm

- https://embear.ch/blog/drm-framebuffer : 分析 drm 实现


如果使用 Tauri，最后是那部分来调用 GPU 的

gnome kde 以及 xface 有啥区别?

## amdgpu

drivers/gpu/drm/amd/amdgpu/

```txt
🧀  tokei .
===============================================================================
 Language            Files        Lines         Code     Comments       Blanks
===============================================================================
 Assembly                3         3077         2851            0          226
 C                     749       719829       546376        73253       100200
 C Header             1363      4308514      3789226       452945        66343
 Makefile               53         2516          866         1257          393
===============================================================================
 Total                2168      5033936      4339319       527455       167162
===============================================================================
```

```txt
meson/
===============================================================================
 Language            Files        Lines         Code     Comments       Blanks
===============================================================================
 C                      15         9239         6853         1027         1359
 C Header               16         2790         2289          335          166
 Makefile                1            9            7            1            1
===============================================================================
 Total                  32        12038         9149         1363         1526
===============================================================================
```

```txt
nouveau/
===============================================================================
 Language            Files        Lines         Code     Comments       Blanks
===============================================================================
 C                     703       157858       118348        20822        18688
 C Header              379        43553        36946         3524         3083
===============================================================================
 Total                1082       201411       155294        24346        21771
===============================================================================
```

```txt
radeon/
===============================================================================
 Language            Files        Lines         Code     Comments       Blanks
===============================================================================
 C                     103       144737       115221        12556        16960
 C Header               97        50743        40537         6066         4140
 Makefile                1           83           56            8           19
===============================================================================
 Total                 201       195563       155814        18630        21119
===============================================================================
```

```txt
rockchip/
===============================================================================
 Language            Files        Lines         Code     Comments       Blanks
===============================================================================
 C                      16        15554        11982          964         2608
 C Header               12         3399         2904          176          319
 Makefile                1           20           13            4            3
===============================================================================
 Total                  29        18973        14899         1144         2930
===============================================================================
```

```txt
i915/
===============================================================================
 Language            Files        Lines         Code     Comments       Blanks
===============================================================================
 Assembly                2          236          208            0           28
 C                     376       326693       232024        40816        53853
 C Header              404        53652        35079        10726         7847
 Makefile                2          433          367           36           30
 Plain Text              1           41            0           31           10
===============================================================================
 Total                 785       381055       267678        51609        61768
===============================================================================
```

居然 a770 显卡也是使用这个驱动

## 集成显卡 和 独立显卡在内存的使用上很不同

看来 intel GPU 和 nvidia GPU 对于内存的理解完全不同

1. intel GPU 存在超多的刷新
2. nvidia 的很少

对于 cgroup 进行压缩的时候，nvidia 可以换出，但是 intel 不可以
/home/martins3/core/draft/chores/thunderbird-idle-page.md

echo 当前内存的一般到到环境中，得到的立刻就是 oom

```txt
[144381.714151] tee invoked oom-killer: gfp_mask=0xcc0(GFP_KERNEL), order=0, oom_score_adj=0
[144381.714156] CPU: 10 PID: 492129 Comm: tee Not tainted 6.5.0 #1-NixOS
[144381.714157] Hardware name: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
[144381.714158] Call Trace:
[144381.714160]  <TASK>
[144381.714162]  dump_stack_lvl+0x47/0x60
[144381.714166]  dump_header+0x4a/0x240
[144381.714168]  oom_kill_process+0xf9/0x190
[144381.714169]  out_of_memory+0x247/0x590
[144381.714170]  mem_cgroup_out_of_memory+0x134/0x150
[144381.714172]  memory_max_write+0x117/0x1e0
[144381.714174]  kernfs_fop_write_iter+0x11f/0x200
[144381.714177]  vfs_write+0x23b/0x420
[144381.714179]  ksys_write+0x6f/0xf0
[144381.714180]  do_syscall_64+0x3b/0x90
[144381.714182]  entry_SYSCALL_64_after_hwframe+0x6e/0xd8
[144381.714184] RIP: 0033:0x7f111af107b4
[144381.714212] Code: 15 69 06 0e 00 f7 d8 64 89 02 48 c7 c0 ff ff ff ff eb b7 0f 1f 00 f3 0f 1e fa 80 3d 4d 8c 0e 00 00 74 13 b8 01 00 00 00 0f 05 <48> 3d 00 f0 ff ff 77 54 c3 0f 1f 00 48 83 ec 28 48 89 54 24 18 48
[144381.714212] RSP: 002b:00007ffd73e6c388 EFLAGS: 00000202 ORIG_RAX: 0000000000000001
[144381.714214] RAX: ffffffffffffffda RBX: 00007ffd73e6c4b0 RCX: 00007f111af107b4
[144381.714219] RDX: 000000000000000a RSI: 00007ffd73e6c4b0 RDI: 0000000000000003
[144381.714219] RBP: 000000000000000a R08: 0000000000000004 R09: 0000000000000001
[144381.714219] R10: 00000000000001b6 R11: 0000000000000202 R12: 000000000000000a
[144381.714220] R13: 0000000000e80510 R14: 000000000000000a R15: 00007f111afedb00
[144381.714221]  </TASK>
[144381.714221] memory: usage 718396kB, limit 488280kB, failcnt 55166
[144381.714222] swap: usage 444712kB, limit 9007199254740988kB, failcnt 0
[144381.714222] Memory cgroup stats for /user.slice/user-1000.slice/user@1000.service/app.slice/app-gnome-thunderbird-265606.scope:
[144381.714285] anon 0
[144381.714285] file 610512896
[144381.714286] kernel 125124608
[144381.714286] kernel_stack 3407872
[144381.714286] pagetables 4939776
[144381.714286] sec_pagetables 0
[144381.714287] percpu 1632
[144381.714287] sock 0
[144381.714287] vmalloc 0
[144381.714287] shmem 610512896
[144381.714287] zswap 113365387
[144381.714288] zswapped 453820416
[144381.714288] file_mapped 0
[144381.714288] file_dirty 0
[144381.714288] file_writeback 0
[144381.714288] swapcached 0
[144381.714289] anon_thp 0
[144381.714289] file_thp 0
[144381.714289] shmem_thp 536870912
[144381.714289] inactive_anon 0
[144381.714289] active_anon 0
[144381.714290] inactive_file 0
[144381.714290] active_file 0
[144381.714290] unevictable 610512896
[144381.714290] slab_reclaimable 540168
[144381.714291] slab_unreclaimable 2622184
[144381.714291] slab 3162352
[144381.714291] workingset_refault_anon 0
[144381.714291] workingset_refault_file 0
[144381.714291] workingset_activate_anon 0
[144381.714292] workingset_activate_file 0
[144381.714292] workingset_restore_anon 0
[144381.714292] workingset_restore_file 0
[144381.714292] workingset_nodereclaim 0
[144381.714292] pgscan 118928
[144381.714293] pgsteal 115833
[144381.714293] pgscan_kswapd 0
[144381.714293] pgscan_direct 118928
[144381.714293] pgscan_khugepaged 0
[144381.714293] pgsteal_kswapd 0
[144381.714294] pgsteal_direct 115833
[144381.714294] pgsteal_khugepaged 0
[144381.714294] pgfault 149007941
[144381.714294] pgmajfault 11
[144381.714295] pgrefill 13
[144381.714295] pgactivate 1
[144381.714295] pgdeactivate 0
[144381.714295] pglazyfree 0
[144381.714295] pglazyfreed 0
[144381.714296] zswpin 0
[144381.714296] zswpout 110796
[144381.714296] thp_fault_alloc 0
[144381.714296] thp_collapse_alloc 0
[144381.714296] Tasks state (memory values in pages):
[144381.714297] [  pid  ]   uid  tgid total_vm      rss pgtables_bytes swapents oom_score_adj name
[144381.714299] [ 265606]  1000 265606  1199236    62118  4153344    98752           100 .thunderbird-wr
[144381.714301] [ 265810]  1000 265810   669458    23762   659456     4352           100 WebExtensions
[144381.714302] [ 265853]  1000 265853      248      128    40960        0           100 external-editor
[144381.714303] [ 265856]  1000 265856   670690    24933   651264     4224           100 Isolated Web Co
[144381.714304] oom-kill:constraint=CONSTRAINT_MEMCG,nodemask=(null),cpuset=user.slice,mems_allowed=0,oom_memcg=/user.slice/user-1000.slice/user@1000.service/app.slice/app-gnome-thunderbird-265606.scope,task_memcg=/user.slice/user-1000.slice/user@1000.service/app.slice/app-gnome-thunderbird-265606.scope,task=.thunderbird-wr,pid=265606,uid=1000
[144381.714468] Memory cgroup out of memory: Killed process 265606 (.thunderbird-wr) total-vm:4796944kB, anon-rss:8988kB, file-rss:239484kB, shmem-rss:0kB, UID:1000 pgtables:4056kB oom_score_adj:100
[144381.722649] tee invoked oom-killer: gfp_mask=0xcc0(GFP_KERNEL), order=0, oom_score_adj=0
[144381.722651] CPU: 11 PID: 492129 Comm: tee Not tainted 6.5.0 #1-NixOS
[144381.722652] Hardware name: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
[144381.722653] Call Trace:
[144381.722654]  <TASK>
[144381.722655]  dump_stack_lvl+0x47/0x60
[144381.722657]  dump_header+0x4a/0x240
[144381.722659]  oom_kill_process+0xf9/0x190
[144381.722660]  out_of_memory+0x247/0x590
[144381.722662]  mem_cgroup_out_of_memory+0x134/0x150
[144381.722663]  memory_max_write+0x117/0x1e0
[144381.722664]  kernfs_fop_write_iter+0x11f/0x200
[144381.722666]  vfs_write+0x23b/0x420
[144381.722668]  ksys_write+0x6f/0xf0
[144381.722669]  do_syscall_64+0x3b/0x90
[144381.722671]  entry_SYSCALL_64_after_hwframe+0x6e/0xd8
[144381.722673] RIP: 0033:0x7f111af107b4
[144381.722687] Code: 15 69 06 0e 00 f7 d8 64 89 02 48 c7 c0 ff ff ff ff eb b7 0f 1f 00 f3 0f 1e fa 80 3d 4d 8c 0e 00 00 74 13 b8 01 00 00 00 0f 05 <48> 3d 00 f0 ff ff 77 54 c3 0f 1f 00 48 83 ec 28 48 89 54 24 18 48
[144381.722688] RSP: 002b:00007ffd73e6c388 EFLAGS: 00000202 ORIG_RAX: 0000000000000001
[144381.722689] RAX: ffffffffffffffda RBX: 00007ffd73e6c4b0 RCX: 00007f111af107b4
[144381.722689] RDX: 000000000000000a RSI: 00007ffd73e6c4b0 RDI: 0000000000000003
[144381.722690] RBP: 000000000000000a R08: 0000000000000004 R09: 0000000000000001
[144381.722690] R10: 00000000000001b6 R11: 0000000000000202 R12: 000000000000000a
[144381.722690] R13: 0000000000e80510 R14: 000000000000000a R15: 00007f111afedb00
[144381.722691]  </TASK>
[144381.722692] memory: usage 716200kB, limit 488280kB, failcnt 60432
[144381.722692] swap: usage 445216kB, limit 9007199254740988kB, failcnt 0
[144381.722693] Memory cgroup stats for /user.slice/user-1000.slice/user@1000.service/app.slice/app-gnome-thunderbird-265606.scope:
[144381.722778] anon 40960
[144381.722779] file 610512896
[144381.722779] kernel 122699776
[144381.722780] kernel_stack 933888
[144381.722780] pagetables 4939776
[144381.722780] sec_pagetables 0
[144381.722780] percpu 1632
[144381.722780] sock 0
[144381.722781] vmalloc 0
[144381.722781] shmem 610512896
[144381.722781] zswap 113404831
[144381.722781] zswapped 454336512
[144381.722782] file_mapped 0
[144381.722782] file_dirty 0
[144381.722782] file_writeback 0
[144381.722782] swapcached 0
[144381.722782] anon_thp 0
[144381.722783] file_thp 0
[144381.722783] shmem_thp 536870912
[144381.722783] inactive_anon 0
[144381.722783] active_anon 0
[144381.722784] inactive_file 0
[144381.722784] active_file 0
[144381.722784] unevictable 610512896
[144381.722784] slab_reclaimable 540168
[144381.722784] slab_unreclaimable 2622184
[144381.722785] slab 3162352
[144381.722785] workingset_refault_anon 141
[144381.722785] workingset_refault_file 0
[144381.722785] workingset_activate_anon 0
[144381.722786] workingset_activate_file 0
[144381.722786] workingset_restore_anon 0
[144381.722786] workingset_restore_file 0
[144381.722786] workingset_nodereclaim 0
[144381.722786] pgscan 119263
[144381.722787] pgsteal 115964
[144381.722787] pgscan_kswapd 0
[144381.722787] pgscan_direct 119263
[144381.722787] pgscan_khugepaged 0
[144381.722787] pgsteal_kswapd 0
[144381.722788] pgsteal_direct 115964
[144381.722788] pgsteal_khugepaged 0
[144381.722788] pgfault 149008289
[144381.722788] pgmajfault 152
[144381.722788] pgrefill 417
[144381.722789] pgactivate 132
[144381.722789] pgdeactivate 0
[144381.722789] pglazyfree 0
[144381.722789] pglazyfreed 0
[144381.722789] zswpin 141
[144381.722790] zswpout 110927
[144381.722790] thp_fault_alloc 0
[144381.722790] thp_collapse_alloc 0
[144381.722790] Tasks state (memory values in pages):
[144381.722791] [  pid  ]   uid  tgid total_vm      rss pgtables_bytes swapents oom_score_adj name
[144381.722791] [ 265810]  1000 265810   669458    23762   659456     4352           100 WebExtensions
[144381.722793] [ 265853]  1000 265853      248      128    40960        0           100 external-editor
[144381.722794] [ 265856]  1000 265856   670690    24933   651264     4224           100 Isolated Web Co
[144381.722795] oom-kill:constraint=CONSTRAINT_MEMCG,nodemask=(null),cpuset=user.slice,mems_allowed=0,oom_memcg=/user.slice/user-1000.slice/user@1000.service/app.slice/app-gnome-thunderbird-265606.scope,task_memcg=/user.slice/user-1000.slice/user@1000.service/app.slice/app-gnome-thunderbird-265606.scope,task=Isolated Web Co,pid=265856,uid=1000
[144381.722813] Memory cgroup out of memory: Killed process 265856 (Isolated Web Co) total-vm:2682760kB, anon-rss:0kB, file-rss:99584kB, shmem-rss:148kB, UID:1000 pgtables:636kB oom_score_adj:100
[144381.727448] tee invoked oom-killer: gfp_mask=0xcc0(GFP_KERNEL), order=0, oom_score_adj=0
[144381.727449] CPU: 11 PID: 492129 Comm: tee Not tainted 6.5.0 #1-NixOS
[144381.727450] Hardware name: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
[144381.727450] Call Trace:
[144381.727451]  <TASK>
[144381.727452]  dump_stack_lvl+0x47/0x60
[144381.727453]  dump_header+0x4a/0x240
[144381.727455]  out_of_memory+0x3a8/0x590
[144381.727456]  mem_cgroup_out_of_memory+0x134/0x150
[144381.727457]  memory_max_write+0x117/0x1e0
[144381.727458]  kernfs_fop_write_iter+0x11f/0x200
[144381.727460]  vfs_write+0x23b/0x420
[144381.727461]  ksys_write+0x6f/0xf0
[144381.727463]  do_syscall_64+0x3b/0x90
[144381.727464]  entry_SYSCALL_64_after_hwframe+0x6e/0xd8
[144381.727466] RIP: 0033:0x7f111af107b4
[144381.727473] Code: 15 69 06 0e 00 f7 d8 64 89 02 48 c7 c0 ff ff ff ff eb b7 0f 1f 00 f3 0f 1e fa 80 3d 4d 8c 0e 00 00 74 13 b8 01 00 00 00 0f 05 <48> 3d 00 f0 ff ff 77 54 c3 0f 1f 00 48 83 ec 28 48 89 54 24 18 48
[144381.727474] RSP: 002b:00007ffd73e6c388 EFLAGS: 00000202 ORIG_RAX: 0000000000000001
[144381.727475] RAX: ffffffffffffffda RBX: 00007ffd73e6c4b0 RCX: 00007f111af107b4
[144381.727475] RDX: 000000000000000a RSI: 00007ffd73e6c4b0 RDI: 0000000000000003
[144381.727476] RBP: 000000000000000a R08: 0000000000000004 R09: 0000000000000001
[144381.727476] R10: 00000000000001b6 R11: 0000000000000202 R12: 000000000000000a
[144381.727477] R13: 0000000000e80510 R14: 000000000000000a R15: 00007f111afedb00
[144381.727478]  </TASK>
[144381.727478] memory: usage 709300kB, limit 488280kB, failcnt 63201
[144381.727478] swap: usage 424572kB, limit 9007199254740988kB, failcnt 0
[144381.727479] Memory cgroup stats for /user.slice/user-1000.slice/user@1000.service/app.slice/app-gnome-thunderbird-265606.scope:
[144381.727531] anon 24576
[144381.727532] file 610512896
[144381.727532] kernel 115490816
[144381.727532] kernel_stack 65536
[144381.727533] pagetables 4911104
[144381.727533] sec_pagetables 0
[144381.727533] percpu 1088
[144381.727533] sock 0
[144381.727533] vmalloc 0
[144381.727534] shmem 610512896
[144381.727534] zswap 107113636
[144381.727534] zswapped 433053696
[144381.727534] file_mapped 0
[144381.727534] file_dirty 0
[144381.727535] file_writeback 0
[144381.727535] swapcached 0
[144381.727535] anon_thp 0
[144381.727535] file_thp 0
[144381.727536] shmem_thp 536870912
[144381.727536] inactive_anon 0
[144381.727536] active_anon 0
[144381.727536] inactive_file 0
[144381.727536] active_file 0
[144381.727537] unevictable 610512896
[144381.727537] slab_reclaimable 540168
[144381.727537] slab_unreclaimable 2613360
[144381.727537] slab 3153528
[144381.727538] workingset_refault_anon 182
[144381.727538] workingset_refault_file 0
[144381.727538] workingset_activate_anon 0
[144381.727538] workingset_activate_file 0
[144381.727538] workingset_restore_anon 0
[144381.727539] workingset_restore_file 0
[144381.727539] workingset_nodereclaim 0
[144381.727539] pgscan 119353
[144381.727539] pgsteal 116009
[144381.727539] pgscan_kswapd 0
[144381.727540] pgscan_direct 119353
[144381.727540] pgscan_khugepaged 0
[144381.727540] pgsteal_kswapd 0
[144381.727540] pgsteal_direct 116009
[144381.727541] pgsteal_khugepaged 0
[144381.727541] pgfault 149008414
[144381.727541] pgmajfault 193
[144381.727541] pgrefill 435
[144381.727541] pgactivate 177
[144381.727542] pgdeactivate 0
[144381.727542] pglazyfree 0
[144381.727542] pglazyfreed 0
[144381.727542] zswpin 182
[144381.727542] zswpout 110972
[144381.727542] thp_fault_alloc 0
[144381.727543] thp_collapse_alloc 0
[144381.727543] Tasks state (memory values in pages):
[144381.727543] [  pid  ]   uid  tgid total_vm      rss pgtables_bytes swapents oom_score_adj name
[144381.727544] Out of memory and no killable processes...
[144543.788014] wlo1: disconnect from AP 78:57:73:58:c8:30 for new auth to 78:57:73:4d:5e:10
[144543.810565] wlo1: authenticate with 78:57:73:4d:5e:10
[144543.813117] wlo1: send auth to 78:57:73:4d:5e:10 (try 1/3)
[144543.841611] wlo1: authenticated
[144543.841682] wlo1: associate with 78:57:73:4d:5e:10 (try 1/3)
[144543.847247] wlo1: RX ReassocResp from 78:57:73:4d:5e:10 (capab=0x1511 status=0 aid=7)
[144543.853041] wlo1: associated
[144544.414954] wlo1: Limiting TX power to 20 (23 - 3) dBm as advertised by 78:57:73:4d:5e:10
```

## 有趣的东西
https://primamateria.github.io/blog/display-manager-origins/

## 奇怪
https://foss-north.se/2020ii/speakers-and-talks.html#sser
- 其中 Getting pixels on screen on Linux: introduction to Kernel Mode Setting

## QEMU 中自己构建的 kernel 无法正常 vnc 显示
问题描述，这个 config 是在 arm 中从 CONFIG_DRM is not set 去掉之后，
会多出来这些 config ，但是这些 config 会让 x86 无法正确显示。

很显然，不知道那个 is not set 是错误的，应该是和 vga 相关。
```txt
# 仅仅打开 virtio-gpu 而已
CONFIG_KCMP=y
CONFIG_DRM=y
# CONFIG_DRM_DEBUG_MM is not set
CONFIG_DRM_KMS_HELPER=y
# CONFIG_DRM_DEBUG_DP_MST_TOPOLOGY_REFS is not set
CONFIG_DRM_FBDEV_EMULATION=y
CONFIG_DRM_FBDEV_OVERALLOC=100
# CONFIG_DRM_FBDEV_LEAK_PHYS_SMEM is not set
# CONFIG_DRM_LOAD_EDID_FIRMWARE is not set
CONFIG_DRM_GEM_SHMEM_HELPER=y
#
# I2C encoder or helper chips
#
# CONFIG_DRM_I2C_CH7006 is not set
# CONFIG_DRM_I2C_SIL164 is not set
# CONFIG_DRM_I2C_NXP_TDA998X is not set
# CONFIG_DRM_I2C_NXP_TDA9950 is not set
# end of I2C encoder or helper chips

#
# ARM devices
#
# CONFIG_DRM_HDLCD is not set
# CONFIG_DRM_MALI_DISPLAY is not set
# CONFIG_DRM_KOMEDA is not set
# end of ARM devices

# CONFIG_DRM_RADEON is not set
# CONFIG_DRM_AMDGPU is not set
# CONFIG_DRM_NOUVEAU is not set
# CONFIG_DRM_XE is not set
# CONFIG_DRM_VGEM is not set
# CONFIG_DRM_VKMS is not set
# CONFIG_DRM_VMWGFX is not set
# CONFIG_DRM_AST is not set
# CONFIG_DRM_MGAG200 is not set
# CONFIG_DRM_QXL is not set
CONFIG_DRM_VIRTIO_GPU=y
CONFIG_DRM_VIRTIO_GPU_KMS=y
CONFIG_DRM_PANEL=y

#
# Display Panels
#
# CONFIG_DRM_PANEL_ARM_VERSATILE is not set
# CONFIG_DRM_PANEL_LVDS is not set
# CONFIG_DRM_PANEL_OLIMEX_LCD_OLINUXINO is not set
# CONFIG_DRM_PANEL_SAMSUNG_S6E88A0_AMS452EF01 is not set
# CONFIG_DRM_PANEL_SAMSUNG_ATNA33XC20 is not set
# CONFIG_DRM_PANEL_SAMSUNG_S6D7AA0 is not set
# CONFIG_DRM_PANEL_SAMSUNG_S6E63M0 is not set
# CONFIG_DRM_PANEL_SAMSUNG_S6E8AA0 is not set
# CONFIG_DRM_PANEL_SEIKO_43WVF1G is not set
# CONFIG_DRM_PANEL_SHARP_LS037V7DW01 is not set
# CONFIG_DRM_PANEL_EDP is not set
# CONFIG_DRM_PANEL_SIMPLE is not set
# end of Display Panels

CONFIG_DRM_BRIDGE=y
CONFIG_DRM_PANEL_BRIDGE=y

#
# Display Interface Bridges
#
# CONFIG_DRM_CHIPONE_ICN6211 is not set
# CONFIG_DRM_CHRONTEL_CH7033 is not set
# CONFIG_DRM_DISPLAY_CONNECTOR is not set
# CONFIG_DRM_ITE_IT6505 is not set
# CONFIG_DRM_LONTIUM_LT8912B is not set
# CONFIG_DRM_LONTIUM_LT9211 is not set
# CONFIG_DRM_LONTIUM_LT9611 is not set
# CONFIG_DRM_LONTIUM_LT9611UXC is not set
# CONFIG_DRM_ITE_IT66121 is not set
# CONFIG_DRM_LVDS_CODEC is not set
# CONFIG_DRM_MEGACHIPS_STDPXXXX_GE_B850V3_FW is not set
# CONFIG_DRM_NWL_MIPI_DSI is not set
# CONFIG_DRM_NXP_PTN3460 is not set
# CONFIG_DRM_PARADE_PS8622 is not set
# CONFIG_DRM_PARADE_PS8640 is not set
# CONFIG_DRM_SAMSUNG_DSIM is not set
# CONFIG_DRM_SIL_SII8620 is not set
# CONFIG_DRM_SII902X is not set
# CONFIG_DRM_SII9234 is not set
# CONFIG_DRM_SIMPLE_BRIDGE is not set
# CONFIG_DRM_THINE_THC63LVD1024 is not set
# CONFIG_DRM_TOSHIBA_TC358762 is not set
# CONFIG_DRM_TOSHIBA_TC358764 is not set
# CONFIG_DRM_TOSHIBA_TC358767 is not set
# CONFIG_DRM_TOSHIBA_TC358768 is not set
# CONFIG_DRM_TOSHIBA_TC358775 is not set
# CONFIG_DRM_TI_DLPC3433 is not set
# CONFIG_DRM_TI_TFP410 is not set
# CONFIG_DRM_TI_SN65DSI83 is not set
# CONFIG_DRM_TI_SN65DSI86 is not set
# CONFIG_DRM_TI_TPD12S015 is not set
# CONFIG_DRM_ANALOGIX_ANX6345 is not set
# CONFIG_DRM_ANALOGIX_ANX78XX is not set
# CONFIG_DRM_ANALOGIX_ANX7625 is not set
# CONFIG_DRM_I2C_ADV7511 is not set
# CONFIG_DRM_CDNS_DSI is not set
# CONFIG_DRM_CDNS_MHDP8546 is not set
# end of Display Interface Bridges

# CONFIG_DRM_ETNAVIV is not set
# CONFIG_DRM_HISI_HIBMC is not set
# CONFIG_DRM_HISI_KIRIN is not set
# CONFIG_DRM_LOGICVC is not set
# CONFIG_DRM_ARCPGU is not set
# CONFIG_DRM_BOCHS is not set
# CONFIG_DRM_CIRRUS_QEMU is not set
# CONFIG_DRM_SIMPLEDRM is not set
# CONFIG_DRM_PL111 is not set
# CONFIG_DRM_XEN_FRONTEND is not set
# CONFIG_DRM_LIMA is not set
# CONFIG_DRM_PANFROST is not set
# CONFIG_DRM_TIDSS is not set
# CONFIG_DRM_SSD130X is not set
# CONFIG_DRM_POWERVR is not set
CONFIG_HDMI=y
# CONFIG_DRM_ACCEL is not set
CONFIG_VIRTIO_DMA_SHARED_BUFFER=y
```

似乎使用以上的 config 会得到如下的 config 的差异
```diff
- # CONFIG_VT_HW_CONSOLE_BINDING is not set
---
+ CONFIG_VT_HW_CONSOLE_BINDING=y
3417d3416
- # CONFIG_VGA_SWITCHEROO is not set
3424c3423,3425
- # CONFIG_DRM_FBDEV_EMULATION is not set
---
+ CONFIG_DRM_FBDEV_EMULATION=y
+ CONFIG_DRM_FBDEV_OVERALLOC=100
+ # CONFIG_DRM_FBDEV_LEAK_PHYS_SMEM is not set
3531a3533,3546
+ CONFIG_FB_CORE=y
+ # CONFIG_FB_DEVICE is not set
+ CONFIG_FB_CFB_FILLRECT=y
+ CONFIG_FB_CFB_COPYAREA=y
+ CONFIG_FB_CFB_IMAGEBLIT=y
+ CONFIG_FB_SYS_FILLRECT=y
+ CONFIG_FB_SYS_COPYAREA=y
+ CONFIG_FB_SYS_IMAGEBLIT=y
+ CONFIG_FB_SYSMEM_FOPS=y
+ CONFIG_FB_DEFERRED_IO=y
+ CONFIG_FB_IOMEM_FOPS=y
+ CONFIG_FB_IOMEM_HELPERS=y
+ CONFIG_FB_SYSMEM_HELPERS=y
+ CONFIG_FB_SYSMEM_HELPERS_DEFERRED=y
3560a3576,3579
+ CONFIG_FRAMEBUFFER_CONSOLE=y
+ # CONFIG_FRAMEBUFFER_CONSOLE_LEGACY_ACCELERATION is not set
+ CONFIG_FRAMEBUFFER_CONSOLE_DETECT_PRIMARY=y
+ # CONFIG_FRAMEBUFFER_CONSOLE_ROTATION is not set
3561a3581,3582
+
+ # CONFIG_LOGO is not set
5180a5202,5203
+ # CONFIG_FONTS is not set
+ CONFIG_FONT_8x8=y
5182d5204
- CONFIG_FONT_AUTOSELECT=y
```
### 这里其实用几个有意思的问题
1. console 和 vt 的关系
2. 字体
3. fb device 是做啥的 ?
4. 开机的时候的小企鹅如何来的?

## 此外，很多时候，vnc 的窗口会忽然变大，应该是切换了 GPU 吧

## 到底什么是 FB DEV 来着

https://github.com/THU-DSP-LAB/ventus-gpgpu

## kernel 的启动参数这个什么意思
vt.handoff=7

## 设计了
https://github.com/THU-DSP-LAB/ventus-gpgpu

## 有趣
https://www.linaro.org/blog/a-closer-look-at-virtio-and-gpu-virtualisation/
https://linaro.atlassian.net/wiki/spaces/ORKO/pages/28985622530/Building+QEMU+with+virtio-gpu+and+rutabaga+gfx

## 再次开源? 又是什么?
https://news.ycombinator.com/item?id=40988954

https://github.com/srush/GPU-Puzzles

## 看看这两个项目
- [glfw](https://github.com/glfw/glfw) : 只有两三个文件，大约 2 万行，但是支持各种后端，有很多语言的 binding
- [imgui](https://github.com/ocornut/imgui) : 没有太搞清楚这个库的定位

## 啥，HDMI 还需要驱动?
https://news.ycombinator.com/item?id=41386667

https://news.ycombinator.com/item?id=41579268

## 给爷整笑了，原来一直都理解错误了啊
https://askubuntu.com/questions/249150/what-is-kde-gtk-gtk-qt-and-or-gnome

## 一个科普视频
https://www.bilibili.com/video/BV1stxGeYEx4



## sysfs
在物理机中可以看到的:

ls /sys/class/drm
```txt
🧀  l
Permissions Size User Date Modified Name
lrwxrwxrwx     - root 18 Nov 12:16   card1 -> ../../devices/pci0000:00/0000:00:02.0/drm/card1
lrwxrwxrwx     - root 18 Nov 12:16   card1-DP-1 -> ../../devices/pci0000:00/0000:00:02.0/drm/card1/card1-DP-1
lrwxrwxrwx     - root 18 Nov 12:16   card1-HDMI-A-1 -> ../../devices/pci0000:00/0000:00:02.0/drm/card1/card1-HDMI-A-1
lrwxrwxrwx     - root 18 Nov 12:16   card1-HDMI-A-2 -> ../../devices/pci0000:00/0000:00:02.0/drm/card1/card1-HDMI-A-2
lrwxrwxrwx     - root 18 Nov 12:16   renderD128 -> ../../devices/pci0000:00/0000:00:02.0/drm/renderD128
.r--r--r--  4.1k root 18 Nov 12:16   version
```

## ubuntu nvidia GPU 安装真的很简单啊
- https://ubuntu.com/server/docs/nvidia-drivers-installation

## GPU 基本词汇
https://news.ycombinator.com/item?id=42675529

## linux 中依赖的 alsa-lib 是什么东西?

https://news.ycombinator.com/item?id=43440174

## 不错
深入GPU硬件架构及运行机制 - 夕殿萤飞思悄然的文章 - 知乎
https://zhuanlan.zhihu.com/p/357112957
https://news.ycombinator.com/item?id=44365320

## 看看
https://news.ycombinator.com/item?id=44365320

https://www.chipstrat.com/p/gpu-networking-basics-part-1

https://mp.weixin.qq.com/s/zQd4LkRvmNn3N-kAp4Ah6w

## 我可以理解为什么需要网卡需要自己的 rom ，但是 qemu 中的 vga 为什么需要自己的 options rom ?

## 所以，shell 到底是什么意思

https://github.com/noctalia-dev/noctalia-shell

这已经不是普通的 shell 了

## GPU perf 功能
https://mp.weixin.qq.com/s/-K4QkPXY1DKPd6bq5rdrKA?poc_token=HMdHJWijcZY4eTZzFyc3MlBVUpYkAtAfYzbtRElk


## GPU
https://mp.weixin.qq.com/s/9qPD3gXj3HLmrKC64Q6fbQ
```txt
Agentic coding 工具我主要用 Cursor Agent & Claude Code。之前用 Cursor Agent 的时候，我把它运行在一个有 4090 GPU 的 Ubuntu Desktop 上，我只花了 3 个小时就重新设计了一套 GPU DSL，能够在 Ada 架构的 bf16 gemm 上面通过 async load、wmma、swizzeling、pipelining 等技巧实现 cuBLAS 80%~90% 的性能，已然非常惊艳、离谱。我觉得放在几年前，这得花我 3 周时间。假设一个 MIT PhD 三周可以工作 300 小时，这是 100 倍的产出提升。我一直说要做个 Taichi 2.0 解决一下团队平常训 3D foundation model 的性能问题，想等招到比自己更强的人再开始做。结果我招到了 Cursor。（关于 Taichi 2.0 这部分，以后的文章再说。）
```

## 很好，有趣的东西
https://github.com/gpu-mode/lectures
