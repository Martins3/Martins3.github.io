# hwpoison

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
