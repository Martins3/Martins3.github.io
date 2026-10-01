# guest memfd

2026-09-30 : 没有深入调查，问了下 codex 几个问题，跑通了一下实验，只有一点 sense 。

## 解决了什么问题?

guest_memfd 解决的是： guest RAM 必须先成为 QEMU 的用户态内存，才能交给 KVM
使用的设计限制。 它提供一种专门面向 VM 的内存对象，允许 KVM 管理没有 QEMU
用户态映射的 guest 内存。 最直接的需求来自 SNP、TDX 的 private memory。KVM
接口文档 (https://docs.kernel.org/virt/kvm/api.html#kvm-create-guest-memfd)

传统模式中，QEMU 先 mmap() 一块内存，再通过 userspace_addr 注册 memslot。KVM
处理 guest 缺页时，大致这样寻找宿主页：

传统：GPA → memslot → QEMU 的 HVA → GUP／宿主页表 → PFN

gmem 直接绑定：GPA → memslot → guest_memfd + offset → folio／PFN

这里说的是缺页处理时找页的路径；CPU 正常访存仍然走 guest 页表和 EPT。

这个变化主要解决两个问题：

1. private 内存不能按普通进程内存处理。 SNP/TDX 的 guest private
   页需要限制宿主访问，不能继续假设
   QEMU、设备模拟器和普通内存管理代码都能读写这些页。guest_memfd
   可以承载这些页，由 KVM 建立 guest 映射，并管理 shared/private
   状态。加密及对恶意宿主的防护还需要相应硬件支持。设计说明
   (https://lpc.events/event/19/contributions/2147/attachments/1751/3815/2025-12-11-lpc-mm-mc-folio-restructuring-for-huge-pages.pdf)

2. guest 内存生命周期不必再依附 QEMU 的用户页表。 传统模式下，HVA 映射变化会通过
   mmu_notifier 影响 KVM 的 EPT 映射。直接绑定 guest_memfd 后，KVM
   可以围绕文件中的页和 GPA 管理分配、释放、失效及硬件状态，减少与普通进程内存机制的耦合。 对应实现是
   arch/x86/kvm/mmu/mmu.c 中的 kvm_mmu_faultin_pfn_gmem()。

## qemu 的支持

2026-09-30 还是不支持的，下面谈到的内容都是手动添加上了这个 patch 实现的:
https://patchew.org/QEMU/20260812201938.198915-1-michael.roth%40amd.com/

早期主要用于需要 private memory 的机密 VM，例如 SNP、TDX。现在本机 QEMU
补丁还支持普通 VM 的 shared guest_memfd，不能把 guest_memfd 等同于机密 VM。
以下是机密 VM 的 machine 配置示例；是否需要 private guest_memfd 由具体 guest
类型的 `require_guest_memfd` 决定，不能仅凭 `confidential-guest-support` 判断：

```txt
-machine q35,confidential-guest-support=tdx0
-machine q35,confidential-guest-support=sev0
```

## 四种内存模式

这里按 **RAM 后端与 KVM 缺页取页路径** 分成四种模式，不是把 `MMAP` 和
`INIT_SHARED` 的四种位组合当作四种运行模式。

| 模式                                | RAM 后端                                                                 | memslot 注册                                                                    | guest 缺页时如何取得 PFN                                 | QEMU 能否访问 RAM                                            |
| ----------------------------------- | ------------------------------------------------------------------------ | ------------------------------------------------------------------------------- | -------------------------------------------------------- | ------------------------------------------------------------ |
| 1. 传统 HVA                         | 匿名内存、普通 memfd 或其他用户态映射                                    | `userspace_addr`，不设置 `KVM_MEM_GUEST_MEMFD`                                  | GPA → memslot → HVA → GUP／宿主页表 → PFN                | 可以，通过 HVA                                               |
| 2. 机密 VM 的 private/shared 双后端 | private 页由 `flags=0` 的 guest_memfd 承载；shared 页使用另一套 HVA 后端 | 设置 `KVM_MEM_GUEST_MEMFD`，同时注册 `guest_memfd + offset` 和 `userspace_addr` | private → guest_memfd → folio/PFN；shared → HVA → PFN    | 只能直接访问 shared 页；不能直接读写 private guest_memfd 页  |
| 3. Shared guest_memfd 经 HVA        | 整个 RAM 使用 `MMAP \| INIT_SHARED` 创建的 guest_memfd                   | 仍只使用 `userspace_addr`，不直接绑定 fd                                        | GPA → memslot → guest_memfd 的 HVA → GUP／宿主页表 → PFN | 可以，guest_memfd 的 shared 页能映射到用户态                 |
| 4. Shared guest_memfd 直接绑定      | 与模式 3 相同的 shared guest_memfd                                       | 额外设置 `KVM_MEM_GUEST_MEMFD`，绑定同一个 fd 和 offset                         | GPA → memslot → guest_memfd → folio/PFN，绕过 HVA 取页   | 仍然可以 mmap；设备模拟访问与 guest 缺页取页使用同一份后端页 |

**模式 1 与 3 的区别主要是 RAM 从哪里分配；模式 3 与 4 的区别才是 KVM
从哪里取页。** 模式 3 已使用 guest_memfd 承载 RAM，但还沿用传统的 HVA
取页路径；模式 4 才把 memslot 直接绑定到这个内存对象。两者都可以是普通
VM，不表示启用了 SNP/TDX。

### Private/shared 与 mmap 是不同维度

`GUEST_MEMFD_FLAG_MMAP` 允许对 fd 建立 mmap；`GUEST_MEMFD_FLAG_INIT_SHARED`
使文件中的内存初始化为 shared。仅设置 `MMAP` 不意味着 private 页能被宿主访问：
当前 `virt/kvm/guest_memfd.c` 的 `kvm_gmem_fault_user_mapping()` 在没有
`INIT_SHARED` 时返回 `VM_FAULT_SIGBUS`，所以 mmap 建立成功也不等于访问成功。

模式 2 的 private/shared 选择由 GPA 的 `KVM_MEMORY_ATTRIBUTE_PRIVATE` 决定，
它和创建 guest_memfd 的 flags 不是同一个状态。当前 x86 的
`kvm_arch_supports_gmem_init_shared()` 只对不具备 private memory 的 VM 返回真，
因此模式 3、4 的 `INIT_SHARED` 方案不能直接套到 SNP/TDX 的 private VM 上。

### 为什么直接绑定后 shared fault 也不走 HVA

关键不只是注册了 fd，而是 **绑定一个带 `MMAP` 标志的 guest_memfd**：
`kvm_gmem_bind()` 会给 memslot 设置内核内部的 `KVM_MEMSLOT_GMEM_ONLY`。
`arch/x86/kvm/mmu/mmu.c` 中 `__kvm_mmu_faultin_pfn()` 的分支为：

```c
if (fault->is_private || kvm_memslot_is_gmem_only(fault->slot))
    return kvm_mmu_faultin_pfn_gmem(vcpu, fault);
```

模式 2 因此只有 private fault 走 gmem；模式 4 即使是 shared fault，也因
`GMEM_ONLY` 走 gmem。这里不再通过 `userspace_addr` 查找 guest 映射所需的 PFN，
但 QEMU 仍保留 HVA 来加载固件、模拟设备和读写 RAM。改变的是 KVM 取页来源， CPU
正常访存仍然使用 guest 页表和 EPT/NPT。

### 本机 QEMU 的选择与限制

- 模式 2：`machine_require_guest_memfd_private()` 决定是否需要 private 后端；
  `system/physmem.c` 的 `ram_block_add()` 创建额外的 `flags=0` guest_memfd。
- 模式 3：`memory-backend-memfd` 使用 `guest-memfd=on`、`share=on`、`seal=off`、
  `hugetlb=off`，`x-guest-memfd-direct` 保持关闭。
- 模式 4：在模式 3 的基础上设置 `x-guest-memfd-direct=on`。这是本地实验选项，
  不能与 private guest_memfd 后端组合；当前 `memfd_backend_memory_alloc()`
  为它注册了 migration blocker，所以不能把模式 3 的迁移实验结论套到模式 4。

QEMU 的关键分支在 `accel/kvm/kvm-all.c` 的 `kvm_mem_flags()` 和
`kvm_set_phys_mem()`：`RAM_GUEST_MEMFD_PRIVATE` 使用额外的 private fd；
`RAM_GUEST_MEMFD_DIRECT` 使用 RAMBlock 自己的 fd。模式 3 没有这两个标志， 即使
RAMBlock 的 fd 是 guest_memfd，也不会直接绑定给 KVM。


## TODO
[Guest-first memory for KVM](https://mp.weixin.qq.com/s/XqYuS3Btcdf20ipgEtd7Ug)

```txt
config KVM_SW_PROTECTED_VM
	bool "Enable support for KVM software-protected VMs"
	depends on EXPERT
	depends on KVM && X86_64
	select KVM_GENERIC_PRIVATE_MEM
	help
	  Enable support for KVM software-protected VMs.  Currently, software-
	  protected VMs are purely a development and testing vehicle for
	  KVM_CREATE_GUEST_MEMFD.  Attempting to run a "real" VM workload as a
	  software-protected VM will fail miserably.

	  If unsure, say "N".
```

CONFIG_KVM_PRIVATE_MEM

virt/kvm/guest_memfd.c 中:
```txt
static struct file_operations kvm_gmem_fops = {
	.open		= generic_file_open,
	.release	= kvm_gmem_release,
	.fallocate	= kvm_gmem_fallocate,
};
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
