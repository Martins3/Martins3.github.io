# 如何正确的配置 qemu 的 memory 和 cpu
<!-- 8a83e97b-fa34-451b-bc10-713f269ff8df -->

1. arg_mem_cpu+=" -m ${ramsize}G,slots=7,maxmem=${max_ramsize}G"
  1. 中的 slots 是做什么的?
  2. 这里的 maxmem 是如何传递到 guest os 中的。
2. numa 和 socket 居然不是一个体系的，也就是他们可以单独配置的
3. 这里配置的是 max cpu 大于 CPU 的时候，那些存在的 CPU 都是出现在一个的
  5. lscpu -p 的结果也非常奇怪，几乎所有所有
  - arg_mem_cpu+=" -numa cpu,node-id=$numa_id,socket-id=$socket_id,core-id=$((i / 2))"
      - 仔细看看这里还有多少个参数，还没有
4. 似乎现在 memory 都是满的，如果出现内存热插，内存应该插到哪里去?

## 为什么开启了 numa 就不让热插 CPU

有的 numa 上是没有 cpu ，因为 cpu 会先都放到一个
numa 上，然后可以热插的 cpu 放到另外一个 numa 上。

当没有热插 CPU 的时候，就发现所有的 CPU 都是在一个 numa
上，就很鬼畜了。
```txt
numactl -H
available: 2 nodes (0-1)
node 0 cpus: 0 1
node 0 size: 7447 MB
node 0 free: 6364 MB
node 1 cpus:
node 1 size: 8060 MB
node 1 free: 8025 MB
node distances:
node   0   1
  0:  10  20
  1:  20  10
```


## 利用虚拟机测试

每一个 cpu 会有自己的 numa 归属，而且内存会有自己的 numa 归属。

一共 16 个 numa node 的环境做如下两个测试:
```txt
cat /sys/devices/system/node/node*/cpulist
0,8,16,24,32,40,48,56
66,74,82,90,98,106,114,122
67,75,83,91,99,107,115,123
68,76,84,92,100,108,116,124
69,77,85,93,101,109,117,125
70,78,86,94,102,110,118,126
71,79,87,95,103,111,119,127
1,9,17,25,33,41,49,57
2,10,18,26,34,42,50,58
3,11,19,27,35,43,51,59
4,12,20,28,36,44,52,60
5,13,21,29,37,45,53,61
6,14,22,30,38,46,54,62
7,15,23,31,39,47,55,63
64,72,80,88,96,104,112,120
65,73,81,89,97,105,113,121
```

每一个内存所在 numa
```txt
memory/memory999:
total 0
drwxr-xr-x    3 root root    0 Apr 18 22:56 .
drwxr-xr-x 2051 root root    0 Apr 18 22:56 ..
lrwxrwxrwx    1 root root    0 Apr 19 20:08 node11 -> ../../node/node11
-rw-r--r--    1 root root 4096 Apr 19 20:08 online
-r--r--r--    1 root root 4096 Apr 19 20:08 phys_device
-r--r--r--    1 root root 4096 Apr 19 20:08 phys_index
drwxr-xr-x    2 root root    0 Apr 19 20:08 power
-r--r--r--    1 root root 4096 Apr 19 20:08 removable
-rw-r--r--    1 root root 4096 Apr 18 22:56 state
lrwxrwxrwx    1 root root    0 Apr 18 22:56 subsystem -> ../../../../bus/memory
-rw-r--r--    1 root root 4096 Apr 18 22:56 uevent
-r--r--r--    1 root root 4096 Apr 19 20:08 valid_zones
```

但是可以通过 qemu 配置，让两个 cpu 在相同的 numa，但是不同的 socket 吧

## 此外
1. ls /sys/devices/system/memory 那么多目录都是按照什么划分的?
2. 既然有 for_each_online_cpu ，那么就有 for 所有的 cpu ，而非不是

## 看看内核中的支持

arch/x86/include/asm/topology.h

```c
static inline unsigned int topology_max_packages(void)
{
	return __max_logical_packages;
}

static inline unsigned int topology_max_dies_per_package(void)
{
	return __max_dies_per_package;
}

static inline unsigned int topology_num_cores_per_package(void)
{
	return __num_cores_per_package;
}

static inline unsigned int topology_num_threads_per_package(void)
{
	return __num_threads_per_package;
}
```

## vmware 下，默认的 CPU 结构居然是一个 numa node ，多个 socket 的情况
```txt
# The following is the parsable format, which can be fed to other
# programs. Each different item in every column has an unique ID
# starting usually from zero.
# CPU,Core,Socket,Node,,L1d,L1i,L2,L3
0,0,0,0,,0,0,0,0
1,1,1,0,,1,1,1,1
2,2,2,0,,2,2,2,2
3,3,3,0,,3,3,3,3
4,4,4,0,,4,4,4,4
5,5,5,0,,5,5,5,5
6,6,6,0,,6,6,6,6
7,7,7,0,,7,7,7,7
8,8,8,0,,8,8,8,8
9,9,9,0,,9,9,9,9
10,10,10,0,,10,10,10,10
11,11,11,0,,11,11,11,11
12,12,12,0,,12,12,12,12
13,13,13,0,,13,13,13,13
14,14,14,0,,14,14,14,14
15,15,15,0,,15,15,15,15
16,16,16,0,,16,16,16,16
```

这真的不会影响 scheduler 的行为吗?

## qemu 没有警告，但是内核还是有警告的
```txt
[    0.498531][    T0] ------------[ cut here ]------------
[    0.498531][    T0] sched: CPU #1's llc-sibling CPU #0 is not on the same node! [node: 1 != 0]. Ignoring dependency.
[    0.498531][    T0] WARNING: CPU: 1 PID: 0 at arch/x86/kernel/smpboot.c:466 topology_sane.isra.0+0x6b/0x80
[    0.498531][    T0] Modules linked in:
[    0.498531][    T0] CPU: 1 PID: 0 Comm: swapper/1 Not tainted 6.6.0-28.0.0.34.oe2403.x86_64 #1
[    0.498531][    T0] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.16.3-32-g9029a010ec41 04/01/2014
[    0.498531][    T0] RIP: 0010:topology_sane.isra.0+0x6b/0x80
[    0.498531][    T0] Code: 80 3d f2 0a 0c 02 00 75 f2 48 83 ec 08 4c 89 da 44 89 d6 48 c7 c7 b0 f7 50 82 88 44 24 07 c6 05 d4 0a 0c 02 01 e8 f5 5e 09 00 <0f> 0b 0f b6 44 24 07 48 83 c4 08 c3 cc cc cc cc 0f 1f 44 00 00 90
[    0.498531][    T0] RSP: 0000:ffffc900001e3ed0 EFLAGS: 00010082
[    0.498531][    T0] RAX: 0000000000000000 RBX: 0000000000000000 RCX: c0000000fffeffff
[    0.498531][    T0] RDX: 0000000000000000 RSI: 00000000fffeffff RDI: 000000000004fffb
[    0.498531][    T0] RBP: 0000000000000001 R08: 0000000000000000 R09: ffffc900001e3d58
[    0.498531][    T0] R10: 0000000000000003 R11: ffff88a03ffdffe8 R12: 0000000000000001
[    0.498531][    T0] R13: ffff8886374198e0 R14: 0000000000000001 R15: ffff8882374198e0
[    0.498531][    T0] FS:  0000000000000000(0000) GS:ffff888637400000(0000) knlGS:0000000000000000
[    0.498531][    T0] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[    0.498531][    T0] CR2: 0000000000000000 CR3: 0000000002c20000 CR4: 00000000003506e0
[    0.498531][    T0] Call Trace:
[    0.498531][    T0]  <TASK>
[    0.498531][    T0]  ? topology_sane.isra.0+0x6b/0x80
[    0.498531][    T0]  ? __warn+0x7d/0x120
[    0.498531][    T0]  ? topology_sane.isra.0+0x6b/0x80
[    0.498531][    T0]  ? report_bug+0x159/0x180
[    0.498531][    T0]  ? console_unlock+0x53/0xe0
[    0.498531][    T0]  ? handle_bug+0x3c/0x70
[    0.498531][    T0]  ? exc_invalid_op+0x13/0x60
[    0.498531][    T0]  ? asm_exc_invalid_op+0x16/0x20
[    0.498531][    T0]  ? topology_sane.isra.0+0x6b/0x80
[    0.498531][    T0]  set_cpu_sibling_map+0x1de/0x670
[    0.498531][    T0]  start_secondary+0x82/0x140
[    0.498531][    T0]  secondary_startup_64_no_verify+0x18f/0x19b
[    0.498531][    T0]  </TASK>
[    0.498531][    T0] ---[ end trace 0000000000000000 ]---
```

## 启动一个 240 core / 16 numa / 80G 的虚拟机，然后在虚拟机中构建内核

在物理机中观察到的结果为:
```txt
                                       - 77.06% hva_to_pfn                                                                                   ▒
                                          - 76.95% get_user_pages_unlocked                                                                   ▒
                                             - 76.84% __get_user_pages                                                                       ▒
                                                - 76.59% handle_mm_fault                                                                     ▒
                                                   - 76.56% __handle_mm_fault                                                                ▒
                                                      - 76.37% do_numa_page                                                                  ▒
                                                         - 76.20% migrate_misplaced_folio                                                    ▒
                                                            - 76.18% migrate_pages                                                           ▒
                                                               - 76.17% migrate_pages_batch                                                  ▒
                                                                  - 75.89% migrate_folio_unmap                                               ▒
                                                                     - 75.83% try_to_migrate                                                 ▒
                                                                        - 75.83% rmap_walk_anon                                              ▒
                                                                           - 75.79% try_to_migrate_one                                       ▒
                                                                              - 75.58% __mmu_notifier_invalidate_range_start                 ▒
                                                                                 - 75.57% kvm_mmu_notifier_invalidate_range_start            ▒
                                                                                    - 74.37% gfn_to_pfn_cache_invalidate_start               ▒
                                                                                       - 73.25% _raw_spin_lock                               ▒
                                                                                          - 73.24% native_queued_spin_lock_slowpath          ▒
                                                                                             - 0.90% asm_sysvec_apic_timer_interrupt         ▒
                                                                                                - 0.85% sysvec_apic_timer_interrupt          ▒
                                                                                                   - 0.75% __sysvec_apic_timer_interrupt     ▒
                                                                                                      - 0.74% hrtimer_interrupt              ▒
                                                                                                         - 0.65% __hrtimer_run_queues        ▒
                                                                                                            - 0.58% tick_nohz_handler        ▒
                                                                                                                 0.55% update_process_times  ▒
                                                                                         1.00% _raw_read_lock_irq                            ▒
                                                                                    - 1.00% queued_write_lock_slowpath                       ▒
                                                                                         0.98% native_queued_spin_lock_slowpath              ▒
                                   0.69% xa_load                                                                                             ▒
                    3.30% svm_vcpu_run                                                                                                       ▒
                  - 1.75% asm_sysvec_apic_timer_interrupt                                                                                    ▒
                     - 1.60% sysvec_apic_timer_interrupt                                                                                     ▒
                        - 1.42% __sysvec_apic_timer_interrupt
```

去掉 hack_memory_cpu 之后，一切都正常起来了。

## 可以这么配置吗?
```txt
memory-backend='id'
       An alternative to legacy -mem-path and mem-prealloc options.  Allows to use a memory backend as main RAM.

       For example:

          -object memory-backend-file,id=pc.ram,size=512M,mem-path=/hugetlbfs,prealloc=on,share=on
          -machine memory-backend=pc.ram
          -m 512M

       Migration compatibility note:

       • as  backend  id  one  shall use value of 'default-ram-id', advertised by machine type (available via query-machines QMP command), if migration to/from old QEMU (<5.0) is expected.

       • for machine types 4.0 and older, user shall use x-use-canonical-path-for-ramblock-id=off backend option if migration to/from old  QEMU  (<5.0) is expected.

       For example:

          -object memory-backend-ram,id=pc.ram,size=512M,x-use-canonical-path-for-ramblock-id=off
          -machine memory-backend=pc.ram -m 512M
```

如果只是指定一个 numa 可以用 -machine 的方法
TODO 如何理解这里的 -machine memory-backend=mem0 ，奇怪的参数含义，-machine 是这么使用的吗?
arg_mem_cpu+=" -machine memory-backend=mem0 "

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
