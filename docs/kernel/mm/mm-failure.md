# Memory Failure

## 基本操作

按钮:
```txt
/proc/sys/vm/memory_failure_early_kill  (Disabled by default, Will immediately kill processes mapped to the page)
/proc/sys/vm/memory_failure_recovery  (Enabled by default, The page is unmapped/Isolated and when the page is referenced back then it will kill process)
```
操作:


## 文档
https://www.kernel.org/doc/html/latest/mm/hwpoison.html

sudo modprobe hwpoison-inject
echo 0x3e8 > /sys/kernel/debug/hwpoison/corrupt-pfn

```txt
# 随便注入一个
[   83.857915] Injecting memory failure at pfn 0x3e8
[   83.858191] Memory failure: 0x3e8: recovery action for free buddy page: Recovered

# 通过 docs/kernel/mm-failure.sh 注入一个进程的页
[ 7165.324423] Injecting memory failure at pfn 0x17bcde
[ 7165.324824] Memory failure: 0x17bcde: recovery action for dirty LRU page: Recovered
```

但是如果程序去写，那么就可以看到这些内容:
```txt
[197425.968665] Memory failure: 0x40b7719: recovery action for dirty LRU page: Recovered
[197426.771495] MCE: Killing a.out:299207 due to hardware memory corruption fault at 7fea7092d000
```
还引入了 MCE ，那么 aarch64 中恐怕就有不同的效果了。

## 参考
https://blogs.oracle.com/linux/post/memory-ue-handling-and-hwpoison-injection

如何注入
zcat /proc/config.gz | egrep 'CONFIG_DEBUG_FS=y|CONFIG_ACPI_APEI=y|CONFIG_ACPI_APEI_EINJ=m'

```txt
CONFIG_ACPI_APEI=y
CONFIG_ACPI_APEI_GHES=y
CONFIG_ACPI_APEI_MEMORY_FAILURE=y
CONFIG_ACPI_APEI_EINJ=m
```

hygon 机器是可以的，kunpeng 也是可以的:
```txt
🧀  dmesg | grep EINJ
[    0.007486] ACPI: EINJ 0x000000007861A048 000150 (v01 HYGON  HGN EINJ 00000001 HGN  00000001)
[    0.007523] ACPI: Reserving EINJ table memory at [mem 0x7861a048-0x7861a197]
```

### hygon 测试
只能这些内容:
```txt
[root@localhost 14:38:29 einj]$ cat available_error_type
0x00000001      Processor Correctable
0x00000002      Processor Uncorrectable non-fatal
0x00000004      Processor Uncorrectable fatal
```

hygon 注入一个，结果为，机器直接宕机
```txt
sudo modprobe einj
echo 1 | sudo tee /sys/kernel/debug/apei/einj/error_type
echo 1 | sudo tee /sys/kernel/debug/apei/einj/notrigger
echo 1 | sudo tee /sys/kernel/debug/apei/einj/error_inject
```
如果触发 1 ，那么没有任何工作。

### intel

而 Intel(R) Xeon(R) Gold 6230 CPU @ 2.10GHz 上有这些:
```txt
[root@n-201 14:55:08 einj]$ cat available_error_type
0x00000008      Memory Correctable
0x00000010      Memory Uncorrectable non-fatal
0x00000020      Memory Uncorrectable fatal
0x00000040      PCI Express Correctable
0x00000080      PCI Express Uncorrectable non-fatal
0x00000100      PCI Express Uncorrectable fatal
```

```txt
echo 0x10 > /sys/kernel/debug/apei/einj/error_type
echo 0x40e38ca000   > /sys/kernel/debug/apei/einj/param1
echo 0xffffffffffffff00 > /sys/kernel/debug/apei/einj/param2
echo 2 > /sys/kernel/debug/apei/einj/flags
echo 1 > /sys/kernel/debug/apei/einj/notrigger
echo 1 > /sys/kernel/debug/apei/einj/error_inject
```

原来虚拟机中是无法模拟这个的，似乎说的有道理:
https://unix.stackexchange.com/questions/715963/cant-modprobe-einj-in-vm-cloud-services

但是似乎 ARM 是支持的?
```txt
https://patchwork.kernel.org/project/qemu-devel/cover/20190906083152.25716-1-zhengxiang9@huawei.com/#22903991
```

### 67.88 环境
```txt
/sys/kernel/debug/apei/einj # cat available_error_type                                                                                   root@6788
0x00000008      Memory Correctable
0x00000010      Memory Uncorrectable non-fatal
0x00000020      Memory Uncorrectable fatal
```

```txt
echo 0x10 > /sys/kernel/debug/apei/einj/error_type
echo 0x40e38ca000   > /sys/kernel/debug/apei/einj/param1
echo 0xffffffffffffff00 > /sys/kernel/debug/apei/einj/param2
echo 2 > /sys/kernel/debug/apei/einj/flags
echo 1 > /sys/kernel/debug/apei/einj/notrigger
echo 1 > /sys/kernel/debug/apei/einj/error_inject
```
可以得到:
```txt
[198348.652036] {1}[Hardware Error]: Hardware error from APEI Generic Hardware Error Source: 4
[198348.653106] {1}[Hardware Error]: It has been corrected by h/w and requires no further action
[198348.654146] {1}[Hardware Error]: event severity: corrected
[198348.655142] {1}[Hardware Error]:  Error 0, type: corrected
[198348.656123] {1}[Hardware Error]:  fru_text: B1
```

机器直接宕机，因为让 trigger 了:
```txt
echo 0x10 > /sys/kernel/debug/apei/einj/error_type
echo 0x40e38ca000   > /sys/kernel/debug/apei/einj/param1
echo 0xffffffffffffff00 > /sys/kernel/debug/apei/einj/param2
echo 2 > /sys/kernel/debug/apei/einj/flags
# echo 1 > /sys/kernel/debug/apei/einj/notrigger
echo 1 > /sys/kernel/debug/apei/einj/error_inject
```
重启之后:

这个没有任何反应的，差别为使用 error_type

```txt
echo 0x08 > /sys/kernel/debug/apei/einj/error_type
echo 0x40c4ee9000 > /sys/kernel/debug/apei/einj/param1
echo 0xffffffffffffff00 > /sys/kernel/debug/apei/einj/param2
echo 2 > /sys/kernel/debug/apei/einj/flags
echo 1 > /sys/kernel/debug/apei/einj/notrigger
echo 1 > /sys/kernel/debug/apei/einj/error_inject
```

修改为 0x20 之后，系统直接宕机。
```txt
echo 0x20 > /sys/kernel/debug/apei/einj/error_type
echo 0x40c4ee9000 > /sys/kernel/debug/apei/einj/param1
echo 0xfffffffffffff000 > /sys/kernel/debug/apei/einj/param2
echo 2 > /sys/kernel/debug/apei/einj/flags
echo 1 > /sys/kernel/debug/apei/einj/notrigger
echo 1 > /sys/kernel/debug/apei/einj/error_inject
```
在 bmc 的界面中也可以观察到错误。

### kunpeng
```txt
[root@localhost einj]# cat available_error_type
0x00000001      Processor Correctable
0x00000002      Processor Uncorrectable non-fatal
0x00000004      Processor Uncorrectable fatal
0x00000008      Memory Correctable
0x00000010      Memory Uncorrectable non-fatal
0x00000020      Memory Uncorrectable fatal
0x00000040      PCI Express Correctable
0x00000080      PCI Express Uncorrectable non-fatal
0x00000100      PCI Express Uncorrectable fatal
0x00000200      Platform Correctable
0x00000400      Platform Uncorrectable non-fatal
0x00000800      Platform Uncorrectable fatal
```
似乎之前不是这个样子的

echo 0x08 > /sys/kernel/debug/apei/einj/error_type
echo 0x203d1b744000 > /sys/kernel/debug/apei/einj/param1
echo 0xfffffffffffff000 > /sys/kernel/debug/apei/einj/param2
echo 2 > /sys/kernel/debug/apei/einj/flags
echo 1 > /sys/kernel/debug/apei/einj/notrigger
echo 1 > /sys/kernel/debug/apei/einj/error_inject

cat /proc/meminfo 中的: HardwareCorrupted:     0 kB 也不变
## 补充一个 ACPI

解析 cat /sys/firmware/acpi/tables/EINJ 中的内容:

- sudo acpidump -n EINJ -b
- sudo iasl -d einj.dat


## page poison 似乎和这个不是一个东西吧
https://lwn.net/Articles/753261/


触发的代码:

```c
static void
do_sigbus(struct pt_regs *regs, unsigned long error_code, unsigned long address,
	  vm_fault_t fault)
{
	/* Kernel mode? Handle exceptions or die: */
	if (!user_mode(regs)) {
		kernelmode_fixup_or_oops(regs, error_code, address,
					 SIGBUS, BUS_ADRERR, ARCH_DEFAULT_PKEY);
		return;
	}

	/* User-space => ok to do another page fault: */
	if (is_prefetch(regs, error_code, address))
		return;

	sanitize_error_code(address, &error_code);

	if (fixup_vdso_exception(regs, X86_TRAP_PF, error_code, address))
		return;

	set_signal_archinfo(address, error_code);

#ifdef CONFIG_MEMORY_FAILURE
	if (fault & (VM_FAULT_HWPOISON|VM_FAULT_HWPOISON_LARGE)) {
		struct task_struct *tsk = current;
		unsigned lsb = 0;

		pr_err(
	"MCE: Killing %s:%d due to hardware memory corruption fault at %lx\n",
			tsk->comm, tsk->pid, address);
		if (fault & VM_FAULT_HWPOISON_LARGE)
			lsb = hstate_index_to_shift(VM_FAULT_GET_HINDEX(fault));
		if (fault & VM_FAULT_HWPOISON)
			lsb = PAGE_SHIFT;
		force_sig_mceerr(BUS_MCEERR_AR, (void __user *)address, lsb);
		return;
	}
#endif
	force_sig_fault(SIGBUS, BUS_ADRERR, (void __user *)address);
}
```

## 问题，如果一个 page 坏了，是如何隔离的

# hwpoison

对应的文件: mm/memory-failure.c

## kernel doc
url: https://www.kernel.org/doc/html/latest/vm/hwpoison.html

The code consists of a the high level handler in mm/memory-failure.c, a new page poison bit and various checks in the VM to handle poisoned pages.

The main target right now is KVM guests, but it works for all kinds of applications. KVM support requires a recent qemu-kvm release.

For the KVM use there was need for a new signal type so that KVM can inject the machine check into the guest with the proper address.
This in theory allows other applications to handle memory failures too. The expection is that near all applications won’t do that, but some very specialized ones might.

## HWPOISON
url : https://lwn.net/Articles/348886/

While, HWPOISON adopted Intel's usage of the term "poisoning", this should not be confused with the unrelated Linux kernel concept of poisoning: writing a pattern to memory to catch uninitialized memory.

In either case, the hardware doesn't immediately cause a machine check but rather flags the data unit as poisoned until read (or consumed).
Later, when erroneous data is read by executing software, a machine check is initiated. If the erroneous data is never read, no machine check is necessary.
> 将错误推迟，这好吗?

Thus, HWPOISON focuses on memory containment at the page granularity rather than the low granularity supported by Intel's MCA Recovery hardware.

HWPOISON finds the page containing the poisoned data and attempts to isolate this page from further use.
Potentially corrupted processes can then be located by finding all processes that have the corrupted page mapped.

To enable the HWPOISON handler, the kernel configuration parameter `MEMORY_FAILURE` must be set.
Otherwise, hardware poisoning will cause a system panic. Additionally, the architecture must support data poisoning. As of this writing, HWPOISON is enabled for all architectures to make testing on any machine possible via a user-mode fault injector, which is detailed below.

The poisoned bit in the flags field serves as a lock allowing rapid-fire poisoning machine checks on the same page to be handled only once by ignoring subsequent calls to the handler.

Since faulty hardware that supports data poisoning is not easy to come by, a fault injection test harness mm/hwpoison-inject.c has also been developed.
This simple harness uses debugfs to allow failures at an arbitrary page to be injected.

Two VM sysctl parameters are supported by HWPOISON with respect to killing user processes: `vm.memory_failure_early_kill` and `vm.memory_failure_recovery`.
Setting the `vm.memory_failure_early_kill` parameter causes an immediate SIGBUS to be sent to the user process(es).

## Memory poison recovery in khugepaged
url : https://lwn.net/Articles/889134/

We are also careful to unwind operations khuagepaged has performed before
it detects memory failures. For example, before copying and collapsing
a group of anonymous pages into a huge page, the source pages will be
isolated and their page table is unlinked from their PMD. These operations
need to be undone in order to ensure these pages are not changed/lost from
the perspective of other threads (both user and kernel space). As for
file backed memory pages, there already exists a rollback case. This
patch just extends it so that khugepaged also correctly rolls back when
it fails to copy poisoned 4K pages.

> 没看代码，这段不知道什么意思，但是这个 patch 描述的含义非常清楚，khugepage 经常扫描内存，如果遇到，那么就负责将其保留起来。

## 测试工具
- https://www.memtest.org/dev

https://lwn.net/Articles/893565/

## 这个是需要看看的
https://www.youtube.com/watch?v=YVow-qoOJJE

## 如何理解这个代码?
```c
/*
 * Someone wants to read @bytes from a HWPOISON hugetlb @page from @offset.
 * Returns the maximum number of bytes one can read without touching the 1st raw
 * HWPOISON subpage.
 *
 * The implementation borrows the iteration logic from copy_page_to_iter*.
 */
static size_t adjust_range_hwpoison(struct page *page, size_t offset, size_t bytes)
{
	size_t n = 0;
	size_t res = 0;

	/* First subpage to start the loop. */
	page = nth_page(page, offset / PAGE_SIZE);
	offset %= PAGE_SIZE;
	while (1) {
		if (is_raw_hwpoison_page_in_hugepage(page))
			break;

		/* Safe to read n bytes without touching HWPOISON subpage. */
		n = min(bytes, (size_t)PAGE_SIZE - offset);
		res += n;
		bytes -= n;
		if (!bytes || !n)
			break;
		offset += n;
		if (offset == PAGE_SIZE) {
			page = nth_page(page, 1);
			offset = 0;
		}
	}

	return res;
}
```

## 看看
https://blogs.oracle.com/linux/post/memory-ue-handling-and-hwpoison-injection

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
