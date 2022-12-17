# cgroup 一系列的测试脚本
## device.sh
- https://0xax.gitbooks.io/linux-insides/content/Cgroups/linux-cgroups-1.html

## cpuset.sh
https://www.redhat.com/en/blog/world-domination-cgroups-part-6-cpuset

## memcontrol
- https://www.rittmanmead.com/blog/2015/12/using-linux-control-groups-to-constrain-process-memory/

1. 在第一个终端中运行运行

```sh
cgcreate -g memory:mem
watch --interval 1 cgget -g memory:mem # 此时，其中大多数都是 0
```

2. 在另一个 shell 中运行
```sh
cgexec -g memory:mem stress-ng --vm-bytes 150M --vm-keep --vm 1
```

3. 两边都关闭，然后设置 limit
```sh
cgset -r memory.max=100m mem
```

4. 重新运行 stress 之后，可以得到:

```txt
[52243.453783] stress invoked oom-killer: gfp_mask=0xcc0(GFP_KERNEL), order=0, oom_score_adj=0
[52243.453817] CPU: 21 PID: 259541 Comm: stress Kdump: loaded Not tainted 5.18.6 #1-NixOS
[52243.453821] Hardware name: Red Hat KVM, BIOS 1.10.2-3.el7 04/01/2014
[52243.453827] Call Trace:
[52243.453846]  <TASK>
[52243.453868]  dump_stack_lvl+0x45/0x5a
[52243.453908]  dump_header+0x4a/0x1fd
[52243.453916]  oom_kill_process.cold+0xb/0x10
[52243.453919]  out_of_memory+0x236/0x4f0
[52243.453955]  mem_cgroup_out_of_memory+0x136/0x150
[52243.453981]  try_charge_memcg+0x70b/0x7d0
[52243.453990]  ? __alloc_pages+0xe6/0x230
[52243.453999]  charge_memcg+0x83/0xf0
[52243.454001]  __mem_cgroup_charge+0x29/0x80
[52243.454004]  __handle_mm_fault+0xa37/0x1110
[52243.454018]  handle_mm_fault+0xb2/0x280
[52243.454021]  do_user_addr_fault+0x1cc/0x660
[52243.454038]  exc_page_fault+0x67/0x150
[52243.454054]  ? asm_exc_page_fault+0x8/0x30
[52243.454061]  asm_exc_page_fault+0x1e/0x30
[52243.454071] RIP: 0033:0x402ec0
[52243.454085] Code: 8b 54 24 08 31 c0 41 89 dd 85 d2 0f 94 c0 89 44 24 0c 41 83 fe 02 0f 8f fe 00 00 00 31 c0 4d 85 ff 7e 14 0f 1f 80 00 00 00 00 <c6> 44 05 00 5
a 4c 01 e0 49 39 c7 7f f3 48 85 db 0f 84 ec 01 00 00
[52243.454087] RSP: 002b:00007ffd3f8eec50 EFLAGS: 00010206
[52243.454089] RAX: 0000000006381000 RBX: ffffffffffffffff RCX: 00007f4d33df1010
[52243.454091] RDX: 0000000000000001 RSI: 0000000009601000 RDI: 0000000000000000
[52243.454092] RBP: 00007f4d33df1010 R08: 00000000ffffffff R09: 0000000000000000
[52243.454094] R10: 0000000000000022 R11: 0000000000000246 R12: 0000000000001000
[52243.454095] R13: 00000000ffffffff R14: 0000000000000002 R15: 0000000009600000
[52243.454110]  </TASK>
[52243.454114] memory: usage 102400kB, limit 102400kB, failcnt 51
[52243.454115] memory+swap: usage 102400kB, limit 9007199254740988kB, failcnt 0
[52243.454117] kmem: usage 368kB, limit 9007199254740988kB, failcnt 0
[52243.454118] Memory cgroup stats for /mem:
[52243.454137] anon 104472576
               file 0
               kernel 376832
               kernel_stack 16384
               pagetables 282624
               percpu 0
               sock 0
               vmalloc 0
               shmem 0
               file_mapped 0
               file_dirty 0
               file_writeback 0
               swapcached 0
               anon_thp 0
               file_thp 0
               shmem_thp 0
               inactive_anon 104439808
               active_anon 8192
               inactive_file 0
               active_file 0
               unevictable 0
               slab_reclaimable 1824
               slab_unreclaimable 30040
               slab 31864
               workingset_refault_anon 0
               workingset_refault_file 0
               workingset_activate_anon 0
               workingset_activate_file 0
               workingset_restore_anon 0
               workingset_restore_file 0
               workingset_nodereclaim 0
               pgfault 102528
               pgmajfault 0
               pgrefill 0
               pgscan 0
               pgsteal 0
               pgactivate 3
               pgdeactivate 0
               pglazyfree 0
               pglazyfreed 0
               thp_fault_alloc 0
               thp_collapse_alloc 0
[52243.454140] Tasks state (memory values in pages):
[52243.454141] [  pid  ]   uid  tgid total_vm      rss pgtables_bytes swapents oom_score_adj name
[52243.454142] [ 259540]     0 259540      865       78    49152        0             0 stress
[52243.454146] [ 259541]     0 259541    39266    25545   258048        0             0 stress
[52243.454151] oom-kill:constraint=CONSTRAINT_MEMCG,nodemask=(null),cpuset=/,mems_allowed=0,oom_memcg=/mem,task_memcg=/mem,task=stress,pid=259541,uid=0
[52243.454169] Memory cgroup out of memory: Killed process 259541 (stress) total-vm:157064kB, anon-rss:101976kB, file-rss:204kB, shmem-rss:0kB, UID:0 pgtables:252
```

## io
cgcreate -g io:duck
cd /sys/fs/cgroup/duck
echo "8:16  wiops=1000" > io.max
cgexec -g io:duck dd if=/dev/zero of=/dev/sdb bs=1M count=1000

## cpu

- https://oakbytes.wordpress.com/2012/09/02/cgroup-cpu-allocation-cpu-shares-examples/

sudo cgcreate -g cpu:A
sudo cgget -r cpu.shares A
sudo cgexec -g cpu:A dd if=/dev/zero of=/dev/null &
sudo cgset -r cpu.shares=768 A

sudo cgexec -g cpu:C dd if=/dev/zero of=/dev/null &

## TODO
1. https://segmentfault.com/a/1190000007468509

- systemd-cgls : 从 systemd 的架构展示

## 如何切换 cgroup v2 来测试
检测当前是那个版本: https://kubernetes.io/docs/concepts/architecture/cgroups/

```sh
stat -fc %T /sys/fs/cgroup/
```
- tmpfs : v1
- cgroup2fs : v2

```sh
sudo grubby --update-kernel=ALL --args=systemd.unified_cgroup_hierarchy=1
```

老版本的 libcgroup 不能支持 cgroup v2 :
```txt
➜ sudo cgcreate -g cpu:A

[sudo] password for martins3:
cgcreate: libcgroup initialization failed: Cgroup is not mounted
```

## libcgroup 的基本使用手册
centos 8 / openEuler 上手动安装

```sh
yum install autoconf
yum install aclocal
yum install automake
yum install libtool
yum install pam-devel
yum install systemd-devel
```

git clone https://github.com/libcgroup/libcgroup

然后参考此处: https://askubuntu.com/questions/27677/cannot-find-install-sh-install-sh-or-shtool-in-ac-aux
```c
libtoolize --force aclocal
autoheader
automake --force-missing --add-missing
autoconf
```

最后参考官方文档:
```c
./configure; make; make install
```
### 基本的使用方法
1. cgcreate -g memory,hugeltb:duck
