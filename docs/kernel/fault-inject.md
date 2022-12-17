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


- 我不理解，按道理来说，应该
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
