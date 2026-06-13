## copy_to_user
然后简单的分析下如何处理用户态的蛇皮指针之类的问题即可。

最后到达此处
```c
static __always_inline __must_check unsigned long
copy_user_generic(void *to, const void *from, unsigned len)
{
	unsigned ret;

	/*
	 * If CPU has ERMS feature, use copy_user_enhanced_fast_string.
	 * Otherwise, if CPU has rep_good feature, use copy_user_generic_string.
	 * Otherwise, use copy_user_generic_unrolled.
	 */
	alternative_call_2(copy_user_generic_unrolled,
			 copy_user_generic_string,
			 X86_FEATURE_REP_GOOD,
			 copy_user_enhanced_fast_string,
			 X86_FEATURE_ERMS,
			 ASM_OUTPUT2("=a" (ret), "=D" (to), "=S" (from),
				     "=d" (len)),
			 "1" (to), "2" (from), "3" (len)
			 : "memory", "rcx", "r8", "r9", "r10", "r11");
	return ret;
}
```
```c
/*
 * Some CPUs are adding enhanced REP MOVSB/STOSB instructions.
 * It's recommended to use enhanced REP MOVSB/STOSB if it's enabled.
 *
 * Input:
 * rdi destination
 * rsi source
 * rdx count
 *
 * Output:
 * eax uncopied bytes or 0 if successful.
 */
SYM_FUNC_START(copy_user_enhanced_fast_string)
	ASM_STAC
	/* CPUs without FSRM should avoid rep movsb for short copies */
	ALTERNATIVE "cmpl $64, %edx; jb copy_user_short_string", "", X86_FEATURE_FSRM
	movl %edx,%ecx
1:	rep movsb
	xorl %eax,%eax
	ASM_CLAC
	RET

12:	movl %ecx,%edx		/* ecx is zerorest also */
	jmp .Lcopy_user_handle_tail

	_ASM_EXTABLE_CPY(1b, 12b)
SYM_FUNC_END(copy_user_enhanced_fast_string)
```

## 分析下 get_user 和 copy_from_user 的区别

kvm_arch_tsc_set_attr 中
```c
		if (get_user(offset, uaddr))
			break;
```

看上去 get_user 只是一个简单的版本

## 实现的基本原理
https://stackoverflow.com/questions/8265657/how-does-copy-from-user-from-the-linux-kernel-work-internally

特殊的链接方法

## 找到这个 tracepoint 的位置
```txt
sudo bpftrace -e "tracepoint:exceptions:page_fault_kernel { @[kstack] = count(); }"
[sudo] password for martins3:
Attaching 1 probe...
^C

@[
    exc_page_fault+315
    exc_page_fault+315
    asm_exc_page_fault+38
    _copy_to_iter+134
    tty_read+228
    vfs_read+662
    ksys_read+111
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

是通过什么机制自动调用的，应该还是有 page fault 吧
只是 bpftrace 可以将他们连起来:
```txt
@[
    __do_fault+1
    do_pte_missing+357
    handle_mm_fault+2121
    do_user_addr_fault+791
    exc_page_fault+137
    asm_exc_page_fault+38
    __get_user_8+17
    test_uaccess+145
    uaccess_store+133
    kernfs_fop_write_iter+240
    vfs_write+892
    ksys_write+114
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

## 问题的进一步深入
1. 现在 kvm 搞了一个很复杂的机制处理 access user ，通过 mmu notifier 引用的
2. docs/kernel/blk/aio.md 中分析的 ubuf 问题

### O_DIRECT 中的 ubuf 问题
ubuf 相关问题全部都在 lib/iov_iter.c 中

真的可以 write protect 吗?

解释 docs/kernel/iouring/doc.md 中的问题，为什么
io uring 在使用 O_DIRECT 的时候，就需要 map 到内核中?

ubuf 问题配合 userfault ，太方便了:

如果是 buffer write ，也就是 open 的时候没有 O_DIRECT
```txt
@[
        handle_userfault+5
        shmem_get_folio_gfp+924
        shmem_fault+128
        __do_fault+51
        do_read_fault+297
        do_fault+295
        __handle_mm_fault+800
        handle_mm_fault+324
        do_user_addr_fault+517
        exc_page_fault+106
        asm_exc_page_fault+38
        fault_in_readable+106
        fault_in_iov_iter_readable+74
        generic_perform_write+638
        ext4_buffered_write_iter+104
        aio_write+278
        io_submit_one+223
        __x64_sys_io_submit+136
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

使用 O_DIRECT 的时候:
```txt
@[
        handle_userfault+5
        shmem_get_folio_gfp+924
        shmem_fault+128
        __do_fault+51
        do_read_fault+297
        do_fault+295
        __handle_mm_fault+800
        handle_mm_fault+324
        __get_user_pages+389
        __gup_longterm_locked+195
        gup_fast_fallback+272
        iov_iter_extract_pages+230
        __bio_iov_iter_get_pages+130
        bio_iov_iter_get_pages+60
        iomap_dio_bio_iter+572
        __iomap_dio_rw+827
        iomap_dio_rw+18
        ext4_dio_write_iter+412
        aio_write+278
        io_submit_one+223
        __x64_sys_io_submit+136
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

uffd 的考虑:
```txt
History:        #0
Commit:         37cd0575b8510159992d279c530c05f872990b02
Author:         Lokesh Gidra <lokeshgidra@google.com>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    Tue 15 Dec 2020 11:13:49 AM CST
Committer Date: Wed 16 Dec 2020 04:13:46 AM CST

userfaultfd: add UFFD_USER_MODE_ONLY

Patch series "Control over userfaultfd kernel-fault handling", v6.

This patch series is split from [1].  The other series enables SELinux
support for userfaultfd file descriptors so that its creation and movement
can be controlled.

It has been demonstrated on various occasions that suspending kernel code
execution for an arbitrary amount of time at any access to userspace
memory (copy_from_user()/copy_to_user()/...) can be exploited to change
the intended behavior of the kernel.  For instance, handling page faults
in kernel-mode using userfaultfd has been exploited in [2, 3].  Likewise,
FUSE, which is similar to userfaultfd in this respect, has been exploited
in [4, 5] for similar outcome.

This small patch series adds a new flag to userfaultfd(2) that allows
callers to give up the ability to handle kernel-mode faults with the
resulting UFFD file object.  It then adds a 'user-mode only' option to the
unprivileged_userfaultfd sysctl knob to require unprivileged callers to
use this new flag.

The purpose of this new interface is to decrease the chance of an
unprivileged userfaultfd user taking advantage of userfaultfd to enhance
security vulnerabilities by lengthening the race window in kernel code.

[1] https://lore.kernel.org/lkml/20200211225547.235083-1-dancol@google.com/
[2] https://duasynt.com/blog/linux-kernel-heap-spray
[3] https://duasynt.com/blog/cve-2016-6187-heap-off-by-one-exploit
[4] https://googleprojectzero.blogspot.com/2016/06/exploiting-recursion-in-linux-kernel_20.html
[5] https://bugs.chromium.org/p/project-zero/issues/detail?id=808

This patch (of 2):

userfaultfd handles page faults from both user and kernel code.  Add a new
UFFD_USER_MODE_ONLY flag for userfaultfd(2) that makes the resulting
userfaultfd object refuse to handle faults from kernel mode, treating
these faults as if SIGBUS were always raised, causing the kernel code to
fail with EFAULT.

A future patch adds a knob allowing administrators to give some processes
the ability to create userfaultfd file objects only if they pass
UFFD_USER_MODE_ONLY, reducing the likelihood that these processes will
exploit userfaultfd's ability to delay kernel page faults to open timing
windows for future exploits.

Link: https://lkml.kernel.org/r/20201120030411.2690816-1-lokeshgidra@google.com
Link: https://lkml.kernel.org/r/20201120030411.2690816-2-lokeshgidra@google.com
Signed-off-by: Daniel Colascione <dancol@google.com>
Signed-off-by: Lokesh Gidra <lokeshgidra@google.com>
Reviewed-by: Andrea Arcangeli <aarcange@redhat.com>
Cc: Alexander Viro <viro@zeniv.linux.org.uk>
Cc: <calin@google.com>
Cc: Daniel Colascione <dancol@dancol.org>
Cc: Eric Biggers <ebiggers@kernel.org>
Cc: Iurii Zaikin <yzaikin@google.com>
Cc: Jeff Vander Stoep <jeffv@google.com>
Cc: Jerome Glisse <jglisse@redhat.com>
Cc: "Joel Fernandes (Google)" <joel@joelfernandes.org>
Cc: Johannes Weiner <hannes@cmpxchg.org>
Cc: Jonathan Corbet <corbet@lwn.net>
Cc: Kalesh Singh <kaleshsingh@google.com>
Cc: Kees Cook <keescook@chromium.org>
Cc: Luis Chamberlain <mcgrof@kernel.org>
Cc: Mauro Carvalho Chehab <mchehab+huawei@kernel.org>
Cc: Mel Gorman <mgorman@techsingularity.net>
Cc: Mike Rapoport <rppt@linux.vnet.ibm.com>
Cc: Nitin Gupta <nigupta@nvidia.com>
Cc: Peter Xu <peterx@redhat.com>
Cc: Sebastian Andrzej Siewior <bigeasy@linutronix.de>
Cc: Shaohua Li <shli@fb.com>
Cc: Stephen Smalley <stephen.smalley.work@gmail.com>
Cc: Suren Baghdasaryan <surenb@google.com>
Cc: Vlastimil Babka <vbabka@suse.cz>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

当 qemu 通过 aio 发起请求，其需要写 page ，一样会 userfaultfd 检测到:
```txt
@[
        handle_userfault+5
        __handle_mm_fault+800
        handle_mm_fault+324
        __get_user_pages+389
        __gup_longterm_locked+195
        gup_fast_fallback+272
        iov_iter_extract_pages+230
        __bio_iov_iter_get_pages+130
        bio_iov_iter_get_pages+60
        iomap_dio_bio_iter+572
        __iomap_dio_rw+827
        iomap_dio_rw+18
        ext4_file_read_iter+326
        aio_read+239
        io_submit_one+223
        __x64_sys_io_submit+136
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

## lib/iov_iter.c
基本处理工作都是集中到这里吧

## 似乎由于 buffer io 的 page 是内核提供的
，如果磁盘无响应，或者提交 io 到 scheduler ，io 没有返回。

如果是 buffer io ，那么程序是可以退出的，但是如果是 direct io ，
程序是不能退出的。应该就是这么回事，到时候在确认下吧。
```txt
cat /proc/6719/stack
[<0>] wait_on_page_bit_killable+0x129/0x360
[<0>] generic_file_buffered_read_pagenotuptodate+0x79/0x540
[<0>] generic_file_buffered_read_get_pages+0x326/0x570
[<0>] generic_file_buffered_read+0xfa/0x4e0
[<0>] blkdev_read_iter+0x44/0x60
[<0>] new_sync_read+0x10d/0x1b0
[<0>] vfs_read+0x14e/0x1b0
[<0>] ksys_read+0x5f/0xe0
[<0>] do_syscall_64+0x3d/0x80
[<0>] entry_SYSCALL_64_after_hwframe+0x67/0xcc
```

```txt
cat /proc/35129/stack
[<0>] exit_aio+0xdd/0xf0
[<0>] mmput.part.0+0x18/0x120
[<0>] exit_mm+0x1c3/0x270
[<0>] do_exit+0x1a9/0x3e0
[<0>] do_group_exit+0x33/0xa0
[<0>] get_signal+0x15b/0x530
[<0>] arch_do_signal_or_restart+0xec/0x1d0
[<0>] exit_to_user_mode_loop+0xda/0x100
[<0>] exit_to_user_mode_prepare+0xa2/0xb0
[<0>] syscall_exit_to_user_mode+0x12/0x40
[<0>] do_syscall_64+0x4d/0x80
[<0>] entry_SYSCALL_64_after_hwframe+0x67/0xcc
```

## 看看这个 tracepoint
exceptions:page_fault_kernel

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
