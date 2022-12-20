# 错误注入

## dm-dust
- https://blogs.oracle.com/linux/post/error-injection-using-dm-dust
  - dm-dust is a Linux kernel module which can be used to simulate the bad  blocks behavior on a physical disk.

## fault-injection
- http://127.0.0.1:3434/fault-injection/index.html#
  - https://lxadm.com/using-fault-injection/ : 直接 echo /sys/block/sdb/sdb1/make-it-fail
  - /home/martins3/core/linux/tools/testing/fault-injection/failcmd.sh

关注一下从这里开始的:
/home/martins3/core/linux/lib/error-inject.c

这个应该是后来没有合并进去:
https://github.com/ionos-enterprise/fault-injection

/sys/kernel/debug/fail_function/inject 中可以插入的位置，除去 syscall 之后，似乎就很少了
```txt
__filemap_add_folio     ERRNO
should_failslab ERRNO
should_fail_alloc_page  TRUE
should_fail_bio ERRNO
```

```plain

```txt
519:ALLOW_ERROR_INJECTION(should_fail_bio, ERRNO);
```


- 似乎一共出现上述模块中放上一些 hook 而已。
```txt
#0  should_fail_ex (attr=attr@entry=0xffffffff82e043c0 <fail_usercopy>, size=size@entry=1, flags=flags@entry=0) at ./arch/x86/include/asm/preempt.h:27
#1  0xffffffff816b84a7 in should_fail (attr=attr@entry=0xffffffff82e043c0 <fail_usercopy>, size=size@entry=1) at lib/fault-inject.c:157
#2  0xffffffff816b84c1 in should_fail_usercopy () at lib/fault-inject-usercopy.c:37
#3  0xffffffff81683c82 in _copy_to_user (to=to@entry=0x7fb0520ceb20, from=from@entry=0xffffc90001dbfe60, n=n@entry=16) at lib/usercopy.c:30
#4  0xffffffff811a60c5 in copy_to_user (n=16, from=0xffffc90001dbfe60, to=0x7fb0520ceb20) at ./include/linux/uaccess.h:169
#5  put_timespec64 (ts=ts@entry=0xffffc90001dbfe80, uts=uts@entry=0x7fb0520ceb20) at kernel/time/time.c:812
#6  0xffffffff8138eef7 in poll_select_finish (end_time=end_time@entry=0xffffc90001dbfef0, p=p@entry=0x7fb0520ceb20, pt_type=pt_type@entry=PT_TIMESPEC, ret=0) at fs/select.c:346
#7  0xffffffff81390b8b in __do_sys_ppoll (sigsetsize=<optimized out>, sigmask=0x0 <fixed_percpu_data>, tsp=0x7fb0520ceb20, nfds=1, ufds=0x7fb0520ceba0) at fs/select.c:1122
#8  __se_sys_ppoll (sigsetsize=<optimized out>, sigmask=0, tsp=140395267549984, nfds=1, ufds=140395267550112) at fs/select.c:1101
#9  __x64_sys_ppoll (regs=<optimized out>) at fs/select.c:1101
#10 0xffffffff81fc8d08 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001dbff58) at arch/x86/entry/common.c:50
#11 do_syscall_64 (regs=0xffffc90001dbff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#12 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

## BPF
但是我们的选项没有打开：

- https://chaos-mesh.org/docs/simulate-kernel-chaos-on-kubernetes/
  - 真不错啊
  - CONFOG_BPF_KPROBE_OVERRIDE
  - 了解一下 bpf 真不错啊

- 使用的工具:
  - https://github.com/chaos-mesh/bpfki
    - 编译不出来

这里说的限制应该是不存在的吧:
```txt
 long bpf_override_return(struct pt_regs *regs, u64 rc)
 	Description
 		Used for error injection, this helper uses kprobes to override
 		the return value of the probed function, and to set it to *rc*.
 		The first argument is the context *regs* on which the kprobe
 		works.

 		This helper works by setting the PC (program counter)
 		to an override function which is run in place of the original
 		probed function. This means the probed function is not run at
 		all. The replacement function just returns with the required
 		value.

 		This helper has security implications, and thus is subject to
 		restrictions. It is only available if the kernel was compiled
 		with the **CONFIG_BPF_KPROBE_OVERRIDE** configuration
 		option, and in this case it only works on functions tagged with
 		**ALLOW_ERROR_INJECTION** in the kernel code.

 		Also, the helper is only available for the architectures having
 		the CONFIG_FUNCTION_ERROR_INJECTION option. As of this writing,
 		x86 architecture is the only one to support this feature.
 	Return
 		0
```
但是似乎这个限制是存在的:
https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md

测试了一下，这个要求确实存在，那么看来 bpf 并没有什么实际上的增强了。

## notifier 是做什么的 ???

## 简单的测试
似乎 swap 挂掉了页无所谓？
```diff
[ 2121.284602] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[ 2121.284650] FAULT_INJECTION: forcing a failure.
               name fail_make_request, interval 0, probability 100, space 0, times -1
[ 2121.284831] Call Trace:
[ 2121.284833]  <TASK>
[ 2121.284834]  dump_stack_lvl+0x38/0x4c
[ 2121.285546]  should_fail_ex.cold+0x32/0x37
[ 2121.285688]  should_fail_bio+0x32/0x40
[ 2121.285821]  submit_bio_noacct+0x94/0x3e0
[ 2121.285961]  __swap_writepage+0x13c/0x480
[ 2121.286102]  pageout+0xcf/0x260
[ 2121.286215]  shrink_folio_list+0x5e2/0xbd0
[ 2121.286358]  shrink_lruvec+0x5f4/0xbe0
[ 2121.286491]  shrink_node+0x2ce/0x6f0
[ 2121.286617]  do_try_to_free_pages+0xd0/0x560
[ 2121.286767]  try_to_free_pages+0xde/0x200
[ 2121.286907]  __alloc_pages_slowpath.constprop.0+0x3a9/0xd20
[ 2121.287098]  ? _raw_spin_unlock_irqrestore+0x1a/0x40
[ 2121.287270]  __alloc_pages+0x21c/0x250
[ 2121.287400]  __folio_alloc+0x16/0x50
[ 2121.287526]  vma_alloc_folio+0xa2/0x370
[ 2121.287659]  __handle_mm_fault+0x8d5/0x12a0
[ 2121.287806]  handle_mm_fault+0xe4/0x2c0
[ 2121.287940]  do_user_addr_fault+0x1c7/0x670
[ 2121.288085]  ? kvm_read_and_reset_apf_flags+0x49/0x60
[ 2121.288259]  exc_page_fault+0x66/0x150
[ 2121.288389]  asm_exc_page_fault+0x26/0x30
[ 2121.288530] RIP: 0033:0x4012ab
[ 2121.288639] Code: 48 c1 f9 3f 48 89 ca 48 29 d0 48 89 c1 ba 1c 00 00 00 be 3f 20 40 00 bf 29 20 40 00 b8 00 00 00 00 e8 a9 fd ff ff 48 8b 45 e0 <c6> 00 78 48 83 45 f8 01 48 8b 45 f8 48 3b 45 f0 0f 8c 4e ff ff ff
[ 2121.289260] RSP: 002b:00007ffff4a45a00 EFLAGS: 00010202
[ 2121.289438] RAX: 00007f36f731c000 RBX: 0000000000000000 RCX: 0000000000000040
[ 2121.289682] RDX: 0000000000000040 RSI: 0000000000000000 RDI: 00007ffff4a454a0
[ 2121.289921] RBP: 00007ffff4a45a30 R08: 0000000000000000 R09: 0000000000000293
[ 2121.290160] R10: 0000000000000000 R11: 0000000000000246 R12: 00007ffff4a45b48
[ 2121.290399] R13: 0000000000401172 R14: 0000000000403e08 R15: 00007f371b725000
[ 2121.290639]  </TASK>
[ 2121.290719] CPU: 2 PID: 70 Comm: kswapd1 Not tainted 6.1.0-10971-g041fae9c105a-dirty #38
[ 2121.291015] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[ 2121.291241] Call Trace:
[ 2121.291328]  <TASK>
[ 2121.291403]  dump_stack_lvl+0x38/0x4c
[ 2121.291534]  should_fail_ex.cold+0x32/0x37
[ 2121.291675]  should_fail_bio+0x32/0x40
[ 2121.291806]  submit_bio_noacct+0x94/0x3e0
[ 2121.291945]  __swap_writepage+0x13c/0x480
[ 2121.292084]  pageout+0xcf/0x260
[ 2121.292195]  shrink_folio_list+0x5e2/0xbd0
[ 2121.292336]  shrink_lruvec+0x5f4/0xbe0
[ 2121.292467]  shrink_node+0x2ce/0x6f0
[ 2121.292593]  balance_pgdat+0x317/0x6f0
[ 2121.292723]  kswapd+0x1ef/0x3a0
[ 2121.292833]  ? __pfx_autoremove_wake_function+0x10/0x10
[ 2121.293011]  ? __pfx_kswapd+0x10/0x10
[ 2121.293138]  kthread+0xe4/0x110
[ 2121.293250]  ? __pfx_kthread+0x10/0x10
[ 2121.293380]  ret_from_fork+0x29/0x50
[ 2121.293507]  </TASK>
```

相当于不去执行 submit_bio_noacct 而已。

- [ ] 但是 fio 的时候，或者 echo 的时候，必然出现错误的。


- 我不理解，按道理来说，应该的数量绝对不会发生改变才对的
```txt
➜  ~ cat /proc/meminfo | grep Swap
SwapCached:         9752 kB
SwapTotal:       1023996 kB
SwapFree:         404356 kB
```

```txt
➜  share  cat /proc/swaps
Filename                                Type            Size            Used            Priority
/dev/sdb                                partition       1023996         271860          -2
```

## 为什么分配失败，没有接受到 kill 信号

已经无法浮现了！

## 为什么还是存在数据写入到 swap 中
1. fault injection 存在 bug 吗?

是因为我对于错误出现的位置不够好吗?

## 是不是 iscsi debug 的实现机制相同的

可以注入的错误更多，而且:
- end_swap_bio_write 的操作，将 page 重新标记为 dirty 的
  - 似乎这个 page 可以不被放到恶意的操作中

- [ ] 这会导致一个 cgroup 的使用的 page 增加吗?
  - 看上去 current 不会增加，但是其他的会增加!

- end_swap_bio_write
  - SetPageError : 结果什么?
  - ClearPageReclaim : 结果是什么?

什么时候这个 page 应该被 drop 掉:

## 似乎 cgroup 的机制是有问题的

## swap 在什么时候释放 page 的?

错误浮现的步骤:

```sh
cd /sys/bus/pseudo/drivers/scsi_debug
echo 1 > max_luns
echo 1 > add_host

sleep 1
mkswap /dev/sde
swapon /dev/sde

cd /sys/bus/pseudo/drivers/scsi_debug
echo 1 > every_nth
echo 0x10 > opts

cgcreate -g memory:mem
cgset -r memory.max=100m mem

swapoff /dev/vdb3
cd ~/share
gcc a.c && cgexec -g memory:mem  ./a.out
```

哇，我操，为什么又可以恢复了!

echo 1 > /sys/bus/pseudo/drivers/scsi_debug/every_nth
echo 0 > /sys/bus/pseudo/drivers/scsi_debug/every_nth

```sh

cd /sys/kernel/debug/fail_make_request
echo 0 > interval
echo -1 > times
echo 100 > probability

mkswap /dev/sda
swapon /dev/sda
echo 1 > /sys/block/sda/make-it-fail

cgcreate -g memory:mem
cgset -r memory.max=100m mem

swapoff /dev/vdb3
cd ~/share
gcc a.c && cgexec -g memory:mem  ./a.out
```

- [ ] 好像打开一个选项之后，就不再吵闹了

## 会让 scsi_debug 似乎无法正常回复

其大小为 0 :
```txt
sde       8:64   0    0B  0 disk
```

## 将 BLK_WBT 关掉之后，得到的结果是

1. 瞬间 kill 了，效果如下:

但是，看上去完全没有使用过 swap 的，这个现象是为什么?
```txt
#0  send_signal_locked (sig=sig@entry=9, info=info@entry=0x1 <fixed_percpu_data+1>, t=t@entry=0xffff888021fd4300, type=type@entry=PIDTYPE_TGID) at kernel/signal.c:1222
#1  0xffffffff8113e188 in do_send_sig_info (sig=sig@entry=9, info=info@entry=0x1 <fixed_percpu_data+1>, p=p@entry=0xffff888021fd4300, type=type@entry=PIDTYPE_TGID) at kernel/signal.c:1296
#2  0xffffffff812e6fa1 in __oom_kill_process (victim=0xffff888021fd4300, message=0xffffffff829b40c2 "Memory cgroup out of memory") at mm/oom_kill.c:947
#3  0xffffffff812e72dc in oom_kill_process (oc=0xffffc900402efa60, message=0xffffffff829b40c2 "Memory cgroup out of memory") at mm/oom_kill.c:1045
#4  0xffffffff812e7c75 in out_of_memory (oc=oc@entry=0xffffc900402efa60) at mm/oom_kill.c:1174
#5  0xffffffff8139bd51 in mem_cgroup_out_of_memory (memcg=memcg@entry=0xffff8881559c6000, gfp_mask=gfp_mask@entry=1051850, order=order@entry=0) at mm/memcontrol.c:1711
#6  0xffffffff813a0f04 in mem_cgroup_oom (order=0, mask=1051850, memcg=0xffff8881559c6000) at mm/memcontrol.c:1941
#7  try_charge_memcg (memcg=memcg@entry=0xffff8881559c6000, gfp_mask=gfp_mask@entry=1051850, nr_pages=<optimized out>) at mm/memcontrol.c:2736
#8  0xffffffff813a18dd in try_charge (nr_pages=1, gfp_mask=1051850, memcg=0xffff8881559c6000) at mm/memcontrol.c:2830
#9  charge_memcg (folio=folio@entry=0xffffea000460de80, memcg=memcg@entry=0xffff8881559c6000, gfp=gfp@entry=1051850) at mm/memcontrol.c:6947
#10 0xffffffff813a31d8 in __mem_cgroup_charge (folio=folio@entry=0xffffea000460de80, mm=mm@entry=0x0 <fixed_percpu_data>, gfp=gfp@entry=1051850) at mm/memcontrol.c:6968
#11 0xffffffff812de845 in mem_cgroup_charge (mm=0x0 <fixed_percpu_data>, gfp=1051850, folio=0xffffea000460de80) at ./include/linux/memcontrol.h:671
#12 __filemap_add_folio (mapping=mapping@entry=0xffff888118a686f8, folio=folio@entry=0xffffea000460de80, index=index@entry=1, gfp=gfp@entry=1051850, shadowp=shadowp@entry=0xffffc900402efc38) at mm/filemap.c:853
#13 0xffffffff812de966 in filemap_add_folio (mapping=mapping@entry=0xffff888118a686f8, folio=folio@entry=0xffffea000460de80, index=index@entry=1, gfp=gfp@entry=1051850) at mm/filemap.c:935
#14 0xffffffff812e16fc in __filemap_get_folio (mapping=mapping@entry=0xffff888118a686f8, index=index@entry=1, fgp_flags=fgp_flags@entry=68, gfp=1051850) at mm/filemap.c:1977
#15 0xffffffff812e1c30 in filemap_fault (vmf=0xffffc900402efdf8) at mm/filemap.c:3164
#16 0xffffffff81320a9c in __do_fault (vmf=vmf@entry=0xffffc900402efdf8) at mm/memory.c:4163
#17 0xffffffff81325271 in do_read_fault (vmf=0xffffc900402efdf8) at mm/memory.c:4514
#18 do_fault (vmf=vmf@entry=0xffffc900402efdf8) at mm/memory.c:4643
#19 0xffffffff8132a07b in handle_pte_fault (vmf=0xffffc900402efdf8) at mm/memory.c:4931
#20 __handle_mm_fault (vma=vma@entry=0xffff8881164cc428, address=address@entry=4199050, flags=flags@entry=852) at mm/memory.c:5073
#21 0xffffffff8132ae24 in handle_mm_fault (vma=0xffff8881164cc428, address=address@entry=4199050, flags=<optimized out>, flags@entry=852, regs=regs@entry=0xffffc900402eff58) at mm/memory.c:5219
#22 0xffffffff8110d947 in do_user_addr_fault (regs=regs@entry=0xffffc900402eff58, error_code=error_code@entry=20, address=address@entry=4199050) at arch/x86/mm/fault.c:1428
#23 0xffffffff821805a6 in handle_page_fault (address=4199050, error_code=20, regs=0xffffc900402eff58) at arch/x86/mm/fault.c:1519
#24 exc_page_fault (regs=0xffffc900402eff58, error_code=20) at arch/x86/mm/fault.c:1575
#25 0xffffffff82201286 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```
- [ ] 一时间找不到为什么会是 filemap_fault 了
只是因为这次出现了问题
```txt
#0  do_anonymous_page (vmf=0xffffc900402b3df8) at mm/memory.c:4029
#1  handle_pte_fault (vmf=0xffffc900402b3df8) at mm/memory.c:4929
#2  __handle_mm_fault (vma=vma@entry=0xffff88801d6a34c0, address=address@entry=140605407756288, flags=flags@entry=597) at mm/memory.c:5073
#3  0xffffffff8132ae24 in handle_mm_fault (vma=0xffff88801d6a34c0, address=address@entry=140605407756288, flags=<optimized out>, flags@entry=597, regs=regs@entry=0xffffc900402b3f58) at mm/memory.c:5219
#4  0xffffffff8110d947 in do_user_addr_fault (regs=regs@entry=0xffffc900402b3f58, error_code=error_code@entry=6, address=address@entry=140605407756288) at arch/x86/mm/fault.c:1428
#5  0xffffffff821805a6 in handle_page_fault (address=140605407756288, error_code=6, regs=0xffffc900402b3f58) at arch/x86/mm/fault.c:1519
#6  exc_page_fault (regs=0xffffc900402b3f58, error_code=6) at arch/x86/mm/fault.c:1575
#7  0xffffffff82201286 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```

1. 获取 fault 的函数

pburst:init.scoperint (((struct vm_fault *)0xffffc900402b3df8)->vma->vm_file->f_path.dentry->d_iname)
$12 = "a.out\000.events\000-burst:init.scope"

$ print ((struct vm_fault *)0xffffc900403d3df8)->vma->vm_mm->owner->comm
$13 = "a.out\000\000\000\000\000\000\000\000\000\000"

$ print ((struct vm_fault *)0xffffc900403d3df8)->vma->vm_start
$14 = 4198400
$ print ((struct vm_fault *)0xffffc900403d3df8)->vma->vm_end
$15 = 4202496

原来是加载代码段，看上去，其中的结果是，感觉有的 code 的确可以 swap 出去，但是实际上并没有:
$ print  /x ((struct vm_fault *)0xffffc9004053bdf8)->vma->vm_end
$17 = 0x402000
```txt
➜  share gcc a.c && cgexec -g memory:mem2 ./a.out

00400000-00401000 r--p 00000000 00:22 29231568                           /root/share/a.out
00401000-00402000 r-xp 00001000 00:22 29231568                           /root/share/a.out
00402000-00403000 r--p 00002000 00:22 29231568                           /root/share/a.out
00403000-00404000 r--p 00002000 00:22 29231568                           /root/share/a.out
00404000-00405000 rw-p 00003000 00:22 29231568                           /root/share/a.out
008af000-008d0000 rw-p 00000000 00:00 0                                  [heap]
7f2b9a763000-7f2ba3d66000 rw-p 00000000 00:00 0
7f2ba3d66000-7f2ba3d92000 r--p 00000000 fd:12 1314351                    /usr/lib64/libc.so.6
7f2ba3d92000-7f2ba3f01000 r-xp 0002c000 fd:12 1314351                    /usr/lib64/libc.so.6
7f2ba3f01000-7f2ba3f51000 r--p 0019b000 fd:12 1314351                    /usr/lib64/libc.so.6
7f2ba3f51000-7f2ba3f52000 ---p 001eb000 fd:12 1314351                    /usr/lib64/libc.so.6
7f2ba3f52000-7f2ba3f55000 r--p 001eb000 fd:12 1314351                    /usr/lib64/libc.so.6
7f2ba3f55000-7f2ba3f58000 rw-p 001ee000 fd:12 1314351                    /usr/lib64/libc.so.6
7f2ba3f58000-7f2ba3f65000 rw-p 00000000 00:00 0
7f2ba3f73000-7f2ba3f75000 rw-p 00000000 00:00 0
7f2ba3f75000-7f2ba3f77000 r--p 00000000 fd:12 1314348                    /usr/lib64/ld-linux-x86-64.so.2
7f2ba3f77000-7f2ba3f9f000 r-xp 00002000 fd:12 1314348                    /usr/lib64/ld-linux-x86-64.so.2
7f2ba3f9f000-7f2ba3fa9000 r--p 0002a000 fd:12 1314348                    /usr/lib64/ld-linux-x86-64.so.2
7f2ba3faa000-7f2ba3fac000 r--p 00034000 fd:12 1314348                    /usr/lib64/ld-linux-x86-64.so.2
7f2ba3fac000-7f2ba3fae000 rw-p 00036000 fd:12 1314348                    /usr/lib64/ld-linux-x86-64.so.2
7ffc38cbd000-7ffc38cde000 rw-p 00000000 00:00 0                          [stack]
7ffc38cf5000-7ffc38cf9000 r--p 00000000 00:00 0                          [vvar]
7ffc38cf9000-7ffc38cfb000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
```

但是失败的位置总是在此处。

在被 kill 之前，会在此处多次的尝试，似乎没有 BLK_WBT 机制会让这个总是失败:
```txt
#0  __swap_writepage (page=0xffffea00046dd300, wbc=0xffffc900403d38e0) at mm/page_io.c:337
#1  0xffffffff812f649f in pageout (folio=folio@entry=0xffffea00046dd300, mapping=mapping@entry=0xffff88801b354600, plug=plug@entry=0xffffc900403d39a8) at mm/vmscan.c:1298
#2  0xffffffff812f78f2 in shrink_folio_list (folio_list=folio_list@entry=0xffffc900403d3a90, pgdat=pgdat@entry=0xffff88813fffc000, sc=sc@entry=0xffffc900403d3c68, stat=stat@entry=0xffffc900403d3b18, ignore_references=ignore_references@entry=false) at mm/vmscan.c:1947
#3  0xffffffff812f9614 in shrink_inactive_list (lru=LRU_INACTIVE_ANON, sc=0xffffc900403d3c68, lruvec=0xffff888116e12800, nr_to_scan=<optimized out>) at mm/vmscan.c:2526
#4  shrink_list (sc=0xffffc900403d3c68, lruvec=0xffff888116e12800, nr_to_scan=<optimized out>, lru=LRU_INACTIVE_ANON) at mm/vmscan.c:2767
#5  shrink_lruvec (lruvec=lruvec@entry=0xffff888116e12800, sc=sc@entry=0xffffc900403d3c68) at mm/vmscan.c:5954
#6  0xffffffff812f9ede in shrink_node_memcgs (sc=0xffffc900403d3c68, pgdat=0xffff88813fffc000) at mm/vmscan.c:6141
#7  shrink_node (pgdat=pgdat@entry=0xffff88813fffc000, sc=sc@entry=0xffffc900403d3c68) at mm/vmscan.c:6172
#8  0xffffffff812fb120 in shrink_zones (sc=0xffffc900403d3c68, zonelist=<optimized out>) at mm/vmscan.c:6410
#9  do_try_to_free_pages (zonelist=zonelist@entry=0xffff88813fffdb00, sc=sc@entry=0xffffc900403d3c68) at mm/vmscan.c:6472
#10 0xffffffff812fc037 in try_to_free_mem_cgroup_pages (memcg=memcg@entry=0xffff8881199ef000, nr_pages=nr_pages@entry=1, gfp_mask=gfp_mask@entry=3264, reclaim_options=reclaim_options@entry=2, nodemask=nodemask@entry=0x0 <fixed_percpu_data>) at mm/vmscan.c:6789
#11 0xffffffff813a095a in try_charge_memcg (memcg=memcg@entry=0xffff8881199ef000, gfp_mask=gfp_mask@entry=3264, nr_pages=1) at mm/memcontrol.c:2687
#12 0xffffffff813a18dd in try_charge (nr_pages=1, gfp_mask=3264, memcg=0xffff8881199ef000) at mm/memcontrol.c:2830
#13 charge_memcg (folio=folio@entry=0xffffea00046d71c0, memcg=memcg@entry=0xffff8881199ef000, gfp=gfp@entry=3264) at mm/memcontrol.c:6947
#14 0xffffffff813a31d8 in __mem_cgroup_charge (folio=0xffffea00046d71c0, mm=<optimized out>, gfp=gfp@entry=3264) at mm/memcontrol.c:6968
#15 0xffffffff8132a3a8 in mem_cgroup_charge (gfp=3264, mm=<optimized out>, folio=<optimized out>) at ./include/linux/memcontrol.h:671
#16 do_anonymous_page (vmf=0xffffc900403d3df8) at mm/memory.c:4078
#17 handle_pte_fault (vmf=0xffffc900403d3df8) at mm/memory.c:4929
#18 __handle_mm_fault (vma=vma@entry=0xffff88801cc15d10, address=address@entry=140134835929088, flags=flags@entry=597) at mm/memory.c:5073
#19 0xffffffff8132ae24 in handle_mm_fault (vma=0xffff88801cc15d10, address=address@entry=140134835929088, flags=<optimized out>, flags@entry=597, regs=regs@entry=0xffffc900403d3f58) at mm/memory.c:5219
#20 0xffffffff8110d947 in do_user_addr_fault (regs=regs@entry=0xffffc900403d3f58, error_code=error_code@entry=6, address=address@entry=140134835929088) at arch/x86/mm/fault.c:1428
#21 0xffffffff821805a6 in handle_page_fault (address=140134835929088, error_code=6, regs=0xffffc900403d3f58) at arch/x86/mm/fault.c:1519
#22 exc_page_fault (regs=0xffffc900403d3f58, error_code=6) at arch/x86/mm/fault.c:1575
#23 0xffffffff82201286 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```
应该是在此处失败了。

我真的不理解，为什么说，swap 也是使用了 100M 是什么情况：
```txt
[ 3075.130889] memory: usage 102400kB, limit 102400kB, failcnt 684
[ 3075.131093] swap: usage 102728kB, limit 9007199254740988kB, failcnt 0
```

## CONFIG_BLK_WBT 应该是一个完全没有任何作用的 config 了吧!
5f6776ba413ce273f7cb211f1cf8771f0cde7c81 将其挪动了一下位置

git show 87760e5eef359 的时候引入的，当时在考虑 SQ 的问题。

## 为什么 swap 的速度这么慢啊
对不起，是 scsi 的问题

## 尝试注入一下网络

tc qdisc add dev lo root netem delay 100ms
tc qdisc change dev lo root netem delay 1000ms
tc qdisc change dev lo root netem loss 0.1%

为什么 1000ms 的延迟这么大！

## iscsi 原来的盘中注入错误

其实，在 guest 这一侧的效果差不多。
```txt
#19 0xffffffff81759b0b in should_fail (attr=attr@entry=0xffffffff82df2de0 <fail_make_request>, size=<optimized out>) at lib/fault-inject.c:157
#20 0xffffffff816bfe02 in should_fail_request (bytes=<optimized out>, part=<optimized out>) at block/blk-core.c:488
#21 should_fail_bio (bio=bio@entry=0xffff88801b4c2c00) at block/blk-core.c:515
#22 0xffffffff816c0a04 in submit_bio_noacct (bio=0xffff88801b4c2c00) at block/blk-core.c:732
#23 0xffffffff81b7225f in iblock_submit_bios (list=0xffffc900402ffd48) at drivers/target/target_core_iblock.c:383
#24 0xffffffff81b72d90 in iblock_execute_rw (cmd=0xffff88811decacf0, sgl=<optimized out>, sgl_nents=<optimized out>, data_direction=<optimized out>) at drivers/target/target_core_iblock.c:819
#25 0xffffffff81b687f2 in __target_execute_cmd (cmd=cmd@entry=0xffff88811decacf0, do_checks=do_checks@entry=true) at drivers/target/target_core_transport.c:2131
#26 0xffffffff81b689f4 in target_execute_cmd (cmd=0xffff88811decacf0) at drivers/target/target_core_transport.c:2263
#27 0xffffffff81b87a6c in iscsit_check_dataout_payload (cmd=cmd@entry=0xffff88811decab00, hdr=hdr@entry=0xffff8881168fbe80, data_crc_failed=<optimized out>) at drivers/target/iscsi/iscsi_target.c:1723
#28 0xffffffff81b8b1ea in iscsit_handle_data_out (buf=0xffff8881168fbe80 "\005\200", conn=0xffff888117a67800) at drivers/target/iscsi/iscsi_target.c:1751
#29 iscsi_target_rx_opcode (buf=0xffff8881168fbe80 "\005\200", conn=0xffff888117a67800) at drivers/target/iscsi/iscsi_target.c:3992
#30 iscsit_get_rx_pdu (conn=0xffff888117a67800) at drivers/target/iscsi/iscsi_target.c:4159
#31 0xffffffff81b8bdab in iscsi_target_rx_thread (arg=0xffff888117a67800) at drivers/target/iscsi/iscsi_target.c:4189
#32 0xffffffff811546c4 in kthread (_create=0xffff88811bfc3fc0) at kernel/kthread.c:376
#33 0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

## 为什么会将 SIGBUS 信号丢失掉了
因为 vmcore
```txt
#20 0xffffffff816bfe12 in should_fail_request (bytes=<optimized out>, part=<optimized out>) at block/blk-core.c:488
#21 should_fail_bio (bio=bio@entry=0xffff88811cc2cf00) at block/blk-core.c:515
#22 0xffffffff816c0a14 in submit_bio_noacct (bio=0xffff88811cc2cf00) at block/blk-core.c:732
#23 0xffffffff81357f6c in __swap_writepage (page=0xffffea0004943540, wbc=<optimized out>) at mm/page_io.c:368
#24 0xffffffff812f649f in pageout (folio=folio@entry=0xffffea0004943540, mapping=mapping@entry=0xffff88811b25a0c0, plug=plug@entry=0xffffc900402e3298) at mm/vmscan.c:1298
#25 0xffffffff812f78f2 in shrink_folio_list (folio_list=folio_list@entry=0xffffc900402e3380, pgdat=pgdat@entry=0xffff88813fffc000, sc=sc@entry=0xffffc900402e3558, stat=stat@entry=0xffffc900402e3408, ignore_references=ignore_references@entry=false) at mm/vmscan.c:1947
#26 0xffffffff812f9614 in shrink_inactive_list (lru=LRU_INACTIVE_ANON, sc=0xffffc900402e3558, lruvec=0xffff88811be06800, nr_to_scan=<optimized out>) at mm/vmscan.c:2526
#27 shrink_list (sc=0xffffc900402e3558, lruvec=0xffff88811be06800, nr_to_scan=<optimized out>, lru=LRU_INACTIVE_ANON) at mm/vmscan.c:2767
#28 shrink_lruvec (lruvec=lruvec@entry=0xffff88811be06800, sc=sc@entry=0xffffc900402e3558) at mm/vmscan.c:5954
#29 0xffffffff812f9ede in shrink_node_memcgs (sc=0xffffc900402e3558, pgdat=0xffff88813fffc000) at mm/vmscan.c:6141
#30 shrink_node (pgdat=pgdat@entry=0xffff88813fffc000, sc=sc@entry=0xffffc900402e3558) at mm/vmscan.c:6172
#31 0xffffffff812fb120 in shrink_zones (sc=0xffffc900402e3558, zonelist=<optimized out>) at mm/vmscan.c:6410
#32 do_try_to_free_pages (zonelist=zonelist@entry=0xffff88813fffdb00, sc=sc@entry=0xffffc900402e3558) at mm/vmscan.c:6472
#33 0xffffffff812fc037 in try_to_free_mem_cgroup_pages (memcg=memcg@entry=0xffff88811b25d000, nr_pages=nr_pages@entry=1, gfp_mask=gfp_mask@entry=1051850, reclaim_options=reclaim_options@entry=2, nodemask=nodemask@entry=0x0 <fixed_percpu_data>) at mm/vmscan.c:6789
#34 0xffffffff813a096a in try_charge_memcg (memcg=memcg@entry=0xffff88811b25d000, gfp_mask=1051850, gfp_mask@entry=76583872, nr_pages=1) at mm/memcontrol.c:2687
#35 0xffffffff813a18ed in try_charge (nr_pages=1, gfp_mask=76583872, memcg=0xffff88811b25d000) at mm/memcontrol.c:2830
#36 charge_memcg (folio=folio@entry=0xffffea00049093c0, memcg=memcg@entry=0xffff88811b25d000, gfp=gfp@entry=1051850) at mm/memcontrol.c:6947
#37 0xffffffff813a32a4 in mem_cgroup_swapin_charge_folio (folio=folio@entry=0xffffea00049093c0, mm=mm@entry=0x0 <fixed_percpu_data>, gfp=gfp@entry=1051850, entry=..., entry@entry=...) at mm/memcontrol.c:7003
#38 0xffffffff813598e7 in __read_swap_cache_async (entry=entry@entry=..., gfp_mask=gfp_mask@entry=1051850, vma=vma@entry=0xffff88811b1a8260, addr=addr@entry=140156937367552, new_page_allocated=new_page_allocated@entry=0xffffc900402e375e) at mm/swap_state.c:483
#39 0xffffffff81359b5a in swap_cluster_readahead (entry=..., gfp_mask=gfp_mask@entry=1051850, vmf=vmf@entry=0xffffc900402e3850) at mm/swap_state.c:638
#40 0xffffffff8135a0f3 in swapin_readahead (entry=..., entry@entry=..., gfp_mask=gfp_mask@entry=1051850, vmf=vmf@entry=0xffffc900402e3850) at mm/swap_state.c:852
#41 0xffffffff81325a14 in do_swap_page (vmf=vmf@entry=0xffffc900402e3850) at mm/memory.c:3787
#42 0xffffffff8132a28e in handle_pte_fault (vmf=0xffffc900402e3850) at mm/memory.c:4940
#43 __handle_mm_fault (vma=vma@entry=0xffff88811b1a8260, address=address@entry=140156937367552, flags=flags@entry=20) at mm/memory.c:5078
#44 0xffffffff8132ae34 in handle_mm_fault (vma=vma@entry=0xffff88811b1a8260, address=140156937367552, flags=<optimized out>, flags@entry=20, regs=regs@entry=0x0 <fixed_percpu_data>) at mm/memory.c:5224
#45 0xffffffff8131c926 in faultin_page (locked=0xffffc900402e39fc, unshare=<optimized out>, flags=<synthetic pointer>, address=<optimized out>, vma=0xffff88811b1a8260) at mm/gup.c:926
#46 __get_user_pages (mm=mm@entry=0xffff88801aa3d580, start=<optimized out>, start@entry=140156937367552, nr_pages=<optimized out>, nr_pages@entry=1, gup_flags=gup_flags@entry=28, pages=pages@entry=0xffffc900402e3a00, vmas=vmas@entry=0x0 <fixed_percpu_data>, locked=0xffffc900402e39fc) at mm/gup.c:1153
#47 0xffffffff8131efb1 in __get_user_pages_locked (flags=<optimized out>, locked=0xffffc900402e39fc, vmas=0x0 <fixed_percpu_data>, pages=0xffffc900402e3a00, nr_pages=1, start=140156937367552, mm=0xffff88801aa3d580) at mm/gup.c:1373
#48 get_dump_page (addr=addr@entry=140156937367552) at mm/gup.c:1871
#49 0xffffffff8143dcfe in dump_user_range (cprm=cprm@entry=0xffffc900402e3d90, start=<optimized out>, len=<optimized out>) at fs/coredump.c:913
#50 0xffffffff814344f2 in elf_core_dump (cprm=<optimized out>) at fs/binfmt_elf.c:2137
#51 0xffffffff8143d69c in do_coredump (siginfo=siginfo@entry=0xffffc900402e3ec8) at fs/coredump.c:762
#52 0xffffffff811410bb in get_signal (ksig=ksig@entry=0xffffc900402e3ea8) at kernel/signal.c:2845
#53 0xffffffff810caf59 in arch_do_signal_or_restart (regs=0xffffc900402e3f58) at arch/x86/kernel/signal.c:306
#54 0xffffffff811ca0ab in exit_to_user_mode_loop (ti_work=16390, regs=<optimized out>) at kernel/entry/common.c:168
#55 exit_to_user_mode_prepare (regs=0xffffc900402e3f58) at kernel/entry/common.c:203
#56 0xffffffff821849a9 in irqentry_exit_to_user_mode (regs=<optimized out>) at kernel/entry/common.c:309
#57 0xffffffff82201286 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```
