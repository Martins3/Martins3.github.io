# 测试内容

## 基本测试方法

### 如何添加 resource

在 main.c 中将

```c
DEFINE_TESTER(hrtimer)
```

修改为

```c
DEFINE_TESTER_RESOURCE(hrtimer)
```

即可

### workqueue 接口

```c
struct work_plus {
    struct work work;
    // something
};

static void test_logic(struct work_struct *work)
{
    // 如果没有额外的
	struct work_plus *test = (struct work_plus *)work;
}

static int test(void)
{
	return batch_queue_works(test_logic,  2, sizeof(struct work_plus));
}
```

```c
static void test_logic(struct work_struct *work)
{
	struct work *test = (struct work*)work;
}

static int test(void)
{
	return batch_queue_works(test_logic,  2, sizeof(struct work));
}
```

## 测试一下

https://www.kernel.org/doc/html/latest/accounting/index.html

## nullb 的中断可以使用 ipi 来代替吗?

## 理解下 sgs ，为什么非要定义 in out 才可以正常工作

## 如何指定使用哪一个 scheduler 来着？

- 为了支持 cgroup ，似乎需要 scheduler 支持?

## mac 中 virtio iommu 无法正确使用

- viommu_domain_finalise

## 这个公司发了很多 kvm forum ，可以看看

- https://daynix.com/

## https://clickhouse.com/blog/optimizing-clickhouse-intel-high-core-count-cpu
可以看看

## 操作性试验

一个有意思的实践
https://stackoverflow.com/questions/36346835/active-inactive-list-in-linux-kernel?rq=1


## 这个是在讨论什么?

https://lore.kernel.org/netdev/20240729154203.GF3371438@nvidia.com/T/#rbd476f8531476017acd3db80b5c6708b53b10efa

## Hyper-converged

- https://github.com/harvester/harvester 1.8k star
    - 这个是 suse 的一个项目
      - https://docs.harvesterhci.io/v1.1/install/pxe-boot-install/
      - 项目本身没有太大兴趣，而是这个 pxe 安装有趣的哇
        - 当然，也可以看看 rancher k8s 之类的东西

## 自动化控制原理 -> ali 还是 openEuler 的 ai tune 还有 redhat 的 autotune ?

## 看看这些

- https://movie.douban.com/subject/21937445/
- 心灵捕手
- 白日梦想家
  - https://www.bilibili.com/bangumi/play/ep810041

- https://semiengineering.com/intel-vs-samsung-vs-tsmc/

## 如果修改为 250 的话，那么性能会提升多少?
```txt
# CONFIG_HZ_100 is not set
# CONFIG_HZ_250 is not set
# CONFIG_HZ_300 is not set
CONFIG_HZ_1000=y
CONFIG_HZ=1000
```

## 短期计划

- 了解证书相关的原理
- perf book 中形式化验证和验证方法两
  - 先将现在的收集的工具都整理下吧
    - qemu 的 qtomic ，llvm ，kernel kasan 之类的，看看相关的研究吧
- ebpf -> xdp virtio
- https://paulgraham.com/articles.html
- https://hn.algolia.com/?q=orange+pi : 也许购买一个 orange pi 吧
  - 也许通过这个理解中断 dma 之类的吧，有 iommu 吗?

- 看看他的 hacking 路线吧
  - https://jia.je/tags/#rpi3


- [ ] 测试 CO:RE 的功能
  - [ ] 将 bpftool 在不同的地方加载
    - 分析这里的东西 : /home/martins3/core/vn/docs/ebpf/core.md
      - libbpf hub

- https://mysummary.readthedocs.io/zh/latest/%E8%8A%B1%E6%9C%B5%E7%9A%84%E6%B8%A9%E5%AE%A4/README.html
  - 这个人思考的东西很多，很厉害

## 三个环境搭建的问题
1. 还是需要搞一下网络代理，ipxe 无法正常启动的
2. pxe 的启动
3. 证书的问题
   - nvme lts ，岂不是 nvme 要过公网，奇怪啊

## 可以让 funcgraph 仅仅运行一次吗?

## 到底是那个方法?

```txt
exe "make M=./arch/x86/kvm/ modules -j32"
make ./arch/x86/kvm/kvm-intel.ko -j32
```

## 整理一下 qemu 的 hmp qmp shell 的关系是什么？

现在为了找到一个 info ramblock 的实现位置，可难了

```txt
{
    .name       = "ramblock",
    .args_type  = "",
    .params     = "",
    .help       = "Display system ramblock information",
    .cmd_info_hrt = qmp_x_query_ramblock,
},
```

## 测试下 ZONE_DEVICE

## 整理到 copy user 中去

zero_user 函数的实现

## 用用这个
kernel/async.c

## 分析下 aarch64 的中断注册，找个 mini os 测试，也许直接看内核就可以了

看看 stack ，syscall ，expcetion 的差别，percpu 的

## 写一个上下文切换测试出来

然后分析一下代码: kvm 切入切出 syscall 的 context switch

然后对比一下理论。

看看，如果 simd 的处理

应该有人搞过类似的东西了，将这些项目都封装为一个项目吧

## 测试数据在一个 cacheline 导致的问题

继续在 /home/martins3/core/vn/code/module/concurrent/main.c 中测试， 如果两个
thread ，分别操作的数据是相邻的两个 32bit ，64bit，但是在一个 cacheline 中的内容
，以及正好跨 cacheline 的两个 32bit 和 64bit 的，结果如何。

## 测试一下 cacheline 的大小

参考这个项目 : https://github.com/Kobzol/hardware-effects

有什么方便的方法

https://github.com/FedeParola/memory-latency

https://github.com/sudarsunkannan/memlatency : 测试 cache line 的大小

按道理，还可以测试出来

1. page size
2. L1 , L2 , L3 大小
3. 是多少级

## 测试

可以测试指令是几发射的吗?

## 其实 vhost net 非常有趣

这个是用户态进程创建一个 kernel thread ，而且 kernel therad 还是 user thread 的
thread group 中的成员.

也就是 qemu 一旦被 kill ，其中的 vhost thread 也会立刻结束。

## 再次回顾一下
https://blog.csdn.net/maokelong95/article/details/107195192

## 如果写一个驱动，使用 folio_alloc 分配内存

然后 folio_add_lru 添加到 lru 中，会出问题吗? 还是当做 anon page 使用吗?

甚至，folio_add_lru 真的可以添加进去 ?

## 这的确是一个有趣的接口

anon_inode_create_getfile

如果用这个方法创建了文件，那么这个文件什么时候释放的?

## 虽然有点无聊，但是添加测试下

https://www.kernel.org/doc/html/v5.8/arm64/memory.html

## 测试一下这个

还不如直接测试 zstd 这个模块 sg_init_one


## 测试下

对应 mm/process_vm_access.c 中的内容 process_vm_readv

## 都说 ipi 非常的 expensive ，到底有多 expensive 啊?

## 看看
https://github.com/nmenon/kernel_patch_verify

## 把 ept 关闭之后，测试一下 shadow page table

## 测试下 tlb flush asid 的效果

## 测试下 iov_iter_count ，让他和 sg_list 对比一下

似乎用户态也是使用 iov_iter_count

## 这个真的没有内置的功能吗?

https://stackoverflow.com/questions/20069620/print-kernels-page-table-entries

## 看看如何 xen 的使用如何，为什么 linux kernel kvm 中有那么多代码是给 xen 用的

例如 kvm_gpc_activate 在中 arch/x86/kvm/xen.c 被使用

## 可以测试一个 1024 个 core 的 rcu 的效果

## 在虚拟机中测试下: hypercall

https://stackoverflow.com/questions/33590843/implementing-a-custom-hypercall-in-kvm

x86.c: kvm_emulate_hypercall

```c
/* For KVM hypercalls, a three-byte sequence of either the vmcall or the vmmcall
 * instruction.  The hypervisor may replace it with something else but only the
 * instructions are guaranteed to be supported.
 *
 * Up to four arguments may be passed in rbx, rcx, rdx, and rsi respectively.
 * The hypercall number should be placed in rax and the return value will be
 * placed in rax.  No other registers will be clobbered unless explicitly
 * noted by the particular hypercall.
 */

static inline long kvm_hypercall0(unsigned int nr)
{
    long ret;
    asm volatile(KVM_HYPERCALL
             : "=a"(ret)
             : "a"(nr)
             : "memory");
    return ret;
}
```

host 发送 hypercall 的之后，造成从 host 中间退出，然后 最后调用到
kvm_emulate_hypercall, 实际上支持的操作很少

```c
int kvm_emulate_hypercall(struct kvm_vcpu *vcpu)
{
    unsigned long nr, a0, a1, a2, a3, ret;
    int op_64_bit;

    if (kvm_hv_hypercall_enabled(vcpu->kvm))
        return kvm_hv_hypercall(vcpu);
```

在用户态可以调用 hypercall 吗?

## 其实，我们可以写进一个 IOMMU 的封装，似乎，例如 hygon 就有这个问题

- hct_iommu_pfnmap

一个设备，可以通过自己的驱动，将 DMA 空间直接映射给用户态， 而不去借助 vfio
来实现。 对吧。

## 测试一下 mdev 的设备吧

为什么 mdev 会和 iommu 工作到一起

## 看看 percpu 的奇怪的类型转换

lruvec_page_state_local 中:

```txt
for_each_possible_cpu(cpu)
	x += per_cpu(pn_ext->lruvec_stat_local->count[idx], cpu);
```

## 调查一下 memcpy_toio 这个程序

## suse 的 kernel 就是 kernel 环境中使用的吗?

https://github.com/openSUSE/kernel

## 这几个东西都分解一下

watchdog_timer_fn

```txt
/* kick the softlockup detector */
if (completion_done(this_cpu_ptr(&softlockup_completion))) {
	reinit_completion(this_cpu_ptr(&softlockup_completion));
	stop_one_cpu_nowait(smp_processor_id(),
			softlockup_fn, NULL,
			this_cpu_ptr(&softlockup_stop_work));
}
```

## oops 中 backtrace 的问号是什么意思

https://cs4118.github.io/www/2023-1/lect/18-x86-paging.html

## 这种 msr 的 check 是如何被检查到的

所以，写 msr 出错，应该会触发 exception 吧

```txt
[  446.862765] unchecked MSR access error: WRMSR to 0xc0010200 (tried to write 0x000002000053007
6) at rIP: 0xffffffff960603a4 (native_write_msr+0x4/0x20)
[  446.873894] Call Trace:
[  446.874663]  <IRQ>
[  446.875232]  x86_pmu_enable_all+0xbb/0x120
[  446.876689]  svm_hardware_enable+0x192/0x320 [kvm_amd]
[  446.878307]  ? copy_overflow+0x20/0x20 [kvm]
[  446.879720]  kvm_arch_hardware_enable+0x9b/0x290 [kvm]
[  446.881923]  ? copy_overflow+0x20/0x20 [kvm]
[  446.883603]  hardware_enable_nolock+0x2f/0x60 [kvm]
[  446.885351]  flush_smp_call_function_queue+0x56/0x120
[  446.887174]  smp_call_function_interrupt+0x3a/0xd0
[  446.888877]  call_function_interrupt+0xf/0x20
[  446.892477]  </IRQ>
[  447.033493] kvm: SMP vm created on host with unstable TSC; guest TSC will not be reliable
```

## 把 vn 这里的东西清理掉吧，这就是 sched 东西需要被整理的

kernel/lkd/
hack/ps/
hack/namespace.md
hack/process.md

## 但是为什么还有有特殊的 debug option

既然 kernel 已经提供了 danamic_debug 的功能，为什么很多模块还是提供了 debug
option 来调试。

```txt
CONFIG_EXT4_DEBUG:                                                                                                                            │
 │                                                                                                                                               │
 │ Enables run-time debugging support for the ext4 filesystem.                                                                                   │
 │                                                                                                                                               │
 │ If you select Y here, then you will be able to turn on debugging                                                                              │
 │ using dynamic debug control for mb_debug() / ext_debug() msgs.                                                                                │
 │                                                                                                                                               │
 │ Symbol: EXT4_DEBUG [=n]                                                                                                                       │
 │ Type  : bool                                                                                                                                  │
 │ Defined at fs/ext4/Kconfig:96                                                                                                                 │
 │   Prompt: Ext4 debugging support                                                                                                              │
 │   Depends on: BLOCK [=y] && EXT4_FS [=y]                                                                                                      │
 │   Location:                                                                                                                                   │
 │     -> File systems                                                                                                                           │
 │       -> The Extended 4 (ext4) filesystem (EXT4_FS [=y])                                                                                      │
 │         -> Ext4 debugging support (EXT4_DEBUG [=n])
```

## ACPI 的测试环境搭建一下
1. seabios 会使用 ACPI 吗?

## 买一个 ARM 设备，测试一下设备树的功能
直接用 qemu 测试不就可以了吗?

## 让 vmware 用 hyperv 试试吧

## 一个经典问题，既然在 ssd 时代，曾经的 ext4 中的对于性能的优化是没有意义了

## vmtest 也制作一个 initrd，让 vmtest 也可以运行把

按道理来说，不难，提供 initrd 之后，然后启动脚本的最开始的位置使用 先来把需要的
kernel module 都加载上来，overlay fs 来解决。

然后再去手动 chroot ，但是这合理吗?

## 整理掉这些东西

https://linux-kernel-labs.github.io/refs/heads/master/lectures/debugging.html#debug-pagealloc

https://docs.qualcomm.com/bundle/publicresource/topics/80-70015-12/debugging_linux_kernel.html

## 检查下这里的
https://stackoverflow.com/questions/22717661/linux-page-poisoning

## 在 /proc/driver 下只有这个一个

```txt
🧀  cat /proc/driver/rtc
rtc_time        : 13:59:00
rtc_date        : 2025-02-17
alrm_time       : 00:00:00
alrm_date       : 2025-02-18
alarm_IRQ       : no
alrm_pending    : no
update IRQ enabled      : no
periodic IRQ enabled    : no
periodic IRQ frequency  : 1024
max user IRQ frequency  : 64
24hr            : yes
periodic_IRQ    : no
update_IRQ      : no
HPET_emulated   : no
BCD             : yes
DST_enable      : no
periodic_freq   : 1024
batt_status     : okay
```

### ubuntu 直通之后有问题

```txt
-   99.93%     0.00%  qemu-system-x86  libc.so.6                [.] __GI___clone3
     __GI___clone3
     start_thread
     qemu_thread_start
     kvm_vcpu_thread_fn
   - kvm_cpu_exec
      - 99.92% kvm_vcpu_ioctl
         - __GI___ioctl
            - 77.47% entry_SYSCALL_64_after_hwframe
                 do_syscall_64
                 __x64_sys_ioctl
                 kvm_vcpu_ioctl
               - kvm_arch_vcpu_ioctl_run
                  - 39.79% vmx_handle_exit
                     - 38.56% handle_ud
                        - 37.41% x86_emulate_instruction
                           - 34.84% x86_decode_emulated_instruction
                              - 30.94% x86_decode_insn
                                 - 23.90% __do_insn_fetch_bytes
                                    - 13.64% kvm_fetch_guest_virt
                                       - 8.06% __kvm_read_guest_page
                                          - 4.78% __check_object_size
                                             - 1.98% __virt_addr_valid
                                                  0.72% preempt_count_sub
                                               1.50% __check_heap_object
                                       - 1.82% kvm_vcpu_read_guest_page
                                            kvm_vcpu_gfn_to_memslot
                                       - 1.48% vmx_get_cpl
                                            vmx_read_guest_seg_ar
                                         0.86% nonpaging_gva_to_gpa
                                    - 5.13% emulator_get_segment
                                       - 2.83% vmx_get_segment
                                            0.76% vmx_read_guest_seg_selector
                                      1.22% vmx_read_guest_seg_base
                                      0.93% emulator_get_cached_segment_base
                                      0.52% vmx_get_segment_base
                                 - 4.23% emulator_get_segment
                                      2.06% vmx_get_segment
                              - 3.47% init_emulate_ctxt
```

### nvidia 添加了这个目录

```txt
[root@hygon-128-71 14:38:31 nvidia]$ ls
capabilities  gpus  params  patches  registry  suspend  suspend_depth  version  warnings
```

## 编译的基本方法

make SYSSRC=/lib/modules/%{KVERSION}/build/

构建 nvidia 驱动的时候，我们使用这个方法 make SYSSRC=~/data/linux-build/ modules
-j32i

https://askubuntu.com/questions/168279/how-do-i-build-a-single-in-tree-kernel-module

```txt
make SUBDIRS=drivers/staging/ft1000/ft1000-usb modules
```

类似 macro 还有多少个。


## 把 docs/kernel/tutorial/crash.md 整理掉

## 可以测试一下，如果 trace 的点被 lock 保护和没有保护，那么输出的日志是否顺序的

## 显然，softirq 是可以被 kernel ftrace 的，但是 hardirq 就说不定了

## n100 可以继续测试
1. EFI 和 ACPI 都是可以在硬件环境发
2. 把小米笔记本也可以调试
3. 可以测试 xe 驱动了
4. n100 上的显卡似乎可以 sriov 的

## 现在我们已经积累了关于 module 的很多东西了
docs/kernel/module-internal.md 里面的问题都需要整理一下

## memory model 的项目，其实可以通过 deepseek 来生成一下

## 在小米笔记本中

### 有英伟达的显卡和集成显卡，那么 tty0 最后使用的是哪一个显卡来驱动的

### 为什么小米笔记本中就是没有 efi 的?

那么还是使用的 acpi 吗？

## 解决一下小米的散热问题，满功耗运行其实性能还是不错的

## 测试一下 qemu monitor 中的错误注入

pcie_aer_inject_error ，用这个去理解一下 ACPI 相关的内容

mce 是通过 mce 中断吗?

```txt
mce [-b] cpu bank status mcgstatus addr misc -- inject a MCE on the given CPU [and broadcast to other CPUs with -b option]
```

## 直通到虚拟机中测试，if possible !

https://github.com/BBuf/how-to-optim-algorithm-in-cuda

显然是 possible 的，就是现在!

## backtrace 中的问号都是如何产生的

```txt
072.924069] BPF:
[10072.924108] failed to validate module [martins3] BTF: -22
[10143.153303] [martins3:greeter_init:299]
[10143.162655] action = 1 current=tee
[10143.166495] -----------------------
[10143.166596] CPU: 2 UID: 0 PID: 0 Comm: swapper/2 Tainted: G           O       6.13.2-00001-g934999804fb6-dirty #11
[10143.166597] Tainted: [O]=OOT_MODULE
```

## rweverything 来重新审视一下硬件


## 为什么多个程序启动之后，还可以共用一个 unix domain socket ?

swtpm socket

而且 unix domain 还可以被删掉

## 似乎现在来处理 mtrr 已经比较成熟了，可以都一并处理下

/home/martins3/core/vn/docs/kvm/mtrr.md


## 测试一下 __iomem 的效果

需要打开什么选项才可以吗?

## 原来可以直接写 pio

```c
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>

#define PORT_SMI_CMD      0x00b2

int main(void)
{
    if (ioperm(PORT_SMI_CMD, 1, 1) != 0)
	        err(EXIT_FAILURE, "ioperm");

    outb(0x61, PORT_SMI_CMD);
    printf("done\n");

    return EXIT_SUCCESS;
}
```

## 现在 seabios 中累计了太多的东西可以整理一下了

## gdb 可以调试 vdso 吗?

## 其实，回到 linux 桌面环境的唯一问题就是，让 l 和 n 模糊音

## kunpeng 机器编译 qemu

```txt
%Cpu1  : 18.4 us, 81.2 sy,  0.0 ni,  0.3 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st
%Cpu2  : 14.1 us, 85.5 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.3 hi,  0.0 si,  0.0 st
%Cpu3  : 20.7 us, 78.6 sy,  0.0 ni,  0.3 id,  0.0 wa,  0.0 hi,  0.3 si,  0.0 st
%Cpu4  : 67.8 us, 31.6 sy,  0.0 ni,  0.3 id,  0.0 wa,  0.3 hi,  0.0 si,  0.0 st
%Cpu5  : 17.8 us, 81.5 sy,  0.0 ni,  0.3 id,  0.0 wa,  0.3 hi,  0.0 si,  0.0 st
%Cpu6  : 22.0 us, 77.6 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.3 hi,  0.0 si,  0.0 st
%Cpu7  : 26.3 us, 73.0 sy,  0.0 ni,  0.3 id,  0.0 wa,  0.3 hi,  0.0 si,  0.0 st
%Cpu8  : 15.7 us, 83.6 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.3 hi,  0.3 si,  0.0 st                                                                    %Cpu9  : 13.0 us, 87.0 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st
%Cpu10 : 44.2 us, 54.5 sy,  0.0 ni,  1.0 id,  0.0 wa,  0.0 hi,  0.3 si,  0.0 st                                                                    %Cpu11 : 26.3 us, 73.0 sy,  0.0 ni,  0.3 id,  0.0 wa,  0.3 hi,  0.0 si,  0.0 st
%Cpu12 : 27.6 us, 70.4 sy,  0.0 ni,  1.3 id,  0.0 wa,  0.3 hi,  0.3 si,  0.0 st                                                                    %Cpu13 : 13.8 us, 84.2 sy,  0.0 ni,  0.3 id,  0.0 wa,  0.3 hi,  1.3 si,  0.0 st
%Cpu14 : 36.3 us, 61.7 sy,  0.0 ni,  1.7 id,  0.0 wa,  0.3 hi,  0.0 si,  0.0 st                                                                    %Cpu15 : 12.9 us, 86.8 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.0 hi,  0.3 si,  0.0 st
%Cpu16 : 21.2 us, 78.1 sy,  0.0 ni,  0.7 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st                                                                    %Cpu17 : 40.6 us, 59.1 sy,  0.0 ni,  0.3 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st
%Cpu18 : 35.6 us, 63.0 sy,  0.0 ni,  1.0 id,  0.0 wa,  0.3 hi,  0.0 si,  0.0 st                                                                    %Cpu19 : 30.9 us, 68.4 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.3 hi,  0.3 si,  0.0 st
%Cpu20 : 24.4 us, 74.9 sy,  0.0 ni,  0.3 id,  0.0 wa,  0.3 hi,  0.0 si,  0.0 st                                                                    %Cpu21 : 13.2 us, 86.8 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st
%Cpu22 : 22.3 us, 77.1 sy,  0.0 ni,  0.7 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st                                                                    %Cpu23 : 34.2 us, 65.1 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.3 hi,  0.3 si,  0.0 st
%Cpu24 : 12.5 us, 84.3 sy,  0.0 ni,  2.6 id,  0.0 wa,  0.3 hi,  0.3 si,  0.0 st                                                                    %Cpu25 : 21.5 us, 78.2 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.3 hi,  0.0 si,  0.0 st
%Cpu26 : 13.5 us, 85.5 sy,  0.0 ni,  0.7 id,  0.0 wa,  0.3 hi,  0.0 si,  0.0 st                                                                    %Cpu27 : 11.9 us, 87.8 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.3 hi,  0.0 si,  0.0 st
%Cpu28 : 13.4 us, 85.9 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.3 hi,  0.3 si,  0.0 st                                                                    %Cpu29 : 33.0 us, 67.0 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st
%Cpu30 : 32.1 us, 66.6 sy,  0.0 ni,  1.0 id,  0.0 wa,  0.0 hi,  0.3 si,  0.0 st
%Cpu31 : 28.6 us, 70.7 sy,  0.0 ni,  0.0 id,  0.0 wa,  0.3 hi,  0.3 si,  0.0 st
```

而且编译的也很慢，不知道为什么?

编译内核是没有这么多 sys 的。


## 有趣的东西
https://www.reddit.com/r/neovim/comments/1k33pc4/talk_with_maria_solano_neovim_core_maintainer_lsp/

## bpftrace 有无办法一键 dump 一个结构体?

类似 crash 中的 struct 命令

例如观察这个函数的时候:

```c
static ssize_t cpu_partial_show(struct kmem_cache *s, char *buf)
```

现在我想直接把 kmem_cache 这个结构体完全的 dump 下来。

这么一想，一旦通过 sysfs 将 kernel 中的各个结构体的中的地址拿到，
那么就可以方便的 dump 系统中的内容。

kernel 中有无办法遍历所有的 struct device 类型，然后获取到 parent 类型，然后
dump 。

类似 qom ，dump kenrel 中的各个 object ，然后根据需求，画出来其中的 object
的联系.

不过南大的 paper 似乎就是做这个事情，只是通过 gdb 完成的

## 测试一下这个效果

- https://www.aidenleong.com/linux/call-graph

效果辣眼，还是 ftrace 会好点

https://zhuanlan.zhihu.com/p/476605073

## 都是有意思的问题，作为 bonus 吧

### pstate 相关问题都是需要补充 cpu-power 的输出的

理解一下 cpu-power 的原理吧

### 各个模块的 btf 在哪里?

为什么会有 btf 不兼容的警告?

### 看看
https://drive.google.com/file/d/1Kg2IJaB2LY3GgnFt2H05kHU7Ca1au4YP/view

- 什么东西:
  - https://docs.opennebula.io/6.8/provision_clusters/hci_clusters/aws_cluster_ceph.html#aws-cluster-ceph
  - https://help.aliyun.com/zh/ecs/user-guide/elastic-bare-metal-server-overview

## 继续整理一下 todoist 中内容

## 看看这个东西
https://docs.kernel.org/admin-guide/pm/intel_epb.html

## 有时间，继续调查一下
rcu stall 问题，check 这个 uuid : 3bdf1ab2-75a8-4dfd-90c6-3c9d52f53e27

这里有两个 bug :
1. workqueue 应该也被 reset 一下
2. rcustall 中没有必要使用 kvm_check_and_clear_guest_paused 检查

## 解决一下 nixos gnome 的 bing wallpaper 的问题

### 把 vim 输入法的问题重新调整清楚了哈

### 重新把这个安排上，其实不需要那么多空间，就像是虚拟机一样，所有的东西

都通过 nfs offload 到物理机就可以了，还省去了很多麻烦。
按道理其他的物理机都是改这么操作啊，只要是固定不动的机器。

https://asahilinux.org/2025/05/progress-report-6-15/

## 长期的可以做事情

- https://docs.google.com/document/d/18ciw9PbQinAQig2wuJo2RtU3EZKM5WnapcAfZWSdjYs/edit?tab=t.0

- https://docs.google.com/document/d/16xH3cXrv1L01SpQ3QXyAs-HqGx4uB8lWNlTPLw1u5ng/edit?tab=t.0#heading=h.64fb4voopuo0
  - https://docs.google.com/document/d/1T79orVcEn1qC3aeQ-fybvW2RTDDfDCgubrzdiw4YITs/edit?tab=t.0#heading=h.wfhi0u4x2tzo


## http
https://github.com/mistweaverco/kulala.nvim

## 可以把 windows 的 kernel config 整理一个

## qemu 的 release 模式 assert 为什么没有关闭?

## 提取一下其中的内容 : 大约 20 mini

https://cs4118.github.io/dev-guides/kernel-debugging.html

## CONFIG_WATCHDOG

softdog 又是什么东西?

测试一下这个模块中的东西 drivers/watchdog/softdog.c

## zfs 的 ci 看看
https://github.com/openzfs/zfs/actions/runs/15312944684/job/43082351882

## 可以继续尝试 ai 写一些 bcc 程序，还是先都用习惯了再说
写一些大型一点的观测程序，看看基本的能力范围，把 ebpf/ext4 和 ebpf/ra 两个目录整理好

## 看看

关于 nvme 的故障，关于 cpu 利用率:
https://zdyxry.github.io/2025/09/21/Weekly-Issue-%E4%B8%80%E6%A0%B9%E6%96%B0%E9%B2%9C%E7%9A%84%E7%8E%89%E7%B1%B3/

## 程序运的自我修养整理一下

不是删除，而是写成一个可以自动展示所有东西的项目:
/home/martins3/data/vn/code/src/c/elf/13/malloc.c

## 这个写的好

《存内/近存计算》 - ppo丶n的文章 - 知乎
https://zhuanlan.zhihu.com/p/1909751965933626976

《硬件预取入门》 - ppo丶n的文章 - 知乎 https://zhuanlan.zhihu.com/p/28730285478

《虚拟内存的体系结构和操作系统支持》 - ppo丶n的文章 - 知乎
https://zhuanlan.zhihu.com/p/1894728823985141412

这个专栏中的所有的东西都是值得看的

### springer 似乎出了很多小册子
- Hardware and Software Support for Virtualization
- The Memory System You Can’t Avoid It, You Can’t Ignore It,You Can’t Fake It
- Architectural and Operating System Support for Virtual Memory

## 可以尝试写一下 ftrace 的脚本，既然有了 debuginfo ，就可以在任何地方打点了


## 周末累积

- ~/.dotfiles/scripts/nix/env/ublksrv.nix 中的东西都合并起来

- 这个垃圾东西去掉 : https://ohmyposh.dev/docs/configuration/block#force

一个小问题，既然 O_RDONLY ，后面的 644 是做什么的?

```txt
int fd = open(file_path, O_RDONLY | O_DIRECT, 0644);
```

思考一个问题，为什么需要 0664 ，他的影响是什么

确认一下 docs/kernel/mm/mm-userfault.md 问题一，我感觉是内核 bug
，这个进一步会扩展到一个关键问题，如果陷入到这种状态， 我们该如何唤醒。

需要将 docs/kernel/mm/mm-userfault.md 内容梳理一下

- 为什么现在虚拟机中测试盘的性能这么差了，似乎的确，大概有什么特殊设置，
  - 似乎只有系统盘的性能才差的

上交的现代操作系统，逐页看看，写的极好

写一个 demo 测试下，i = i + 1 和 i++ 编译器有无区别，变成汇编之后，性能有无区别

既然，virtio desc table 是被 used 和 avalid 共享，desc table 如何分配的?


## 这个东西
docs/kernel/module/module-visualize.diff

## 积累的有趣问题

2. docs/kernel/blk/mq/ 中就一个问题

- docs/kernel/blk/storage-todo.md
- 但是还是需要靠 virtio/virtio-dummy.c ，但是 qemu 的 pciemu later
  还没实现，dummy virtio 不是 pci 设备，也不可以发送
  - 也许需要参考一下
5. 完成这个测试，fs/fs-lock-test.c ，最后，找一下，是否存在并发删除文件的操作

## 思考一下这个问题

https://news.ycombinator.com/item?id=44560123

## 测试一下 libfs

例如 simple_read_from_buffer

simplefs 中可以测试到 libfs 吗?

为什么 simplefs 没能利用上这个?

## 这个看看
https://www.usenix.org/sites/default/files/conference/protected-files/osdi25_slides_wang_yun.pdf

bio 的测试可以参考一下 loop 设备
实现 sync_page_io 的效果 或者 md_super_write 的效果
似乎两个都是一样的

## 看看推荐算法的原理
https://95152.douyin.com/transparency

## 学习他的调试方法
https://access.redhat.com/solutions/7077348

## 就这个主题写一个总结吧
https://news.ycombinator.com/item?id=44724216

## 为什么 arm 或者 hygon 中，杀掉一个 QEMU 会卡一会?
是服务器的原因吗?

似乎这个就是 oom.c 中的新系统调用就是为了解决这个问题

## 忽然意识到这个东西是需要被重视的
https://news.ycombinator.com/item?id=44779428


## 不太容易的东西
- qemu 的热迁移机制梳理
- 85ec5100-11db-4271-b647-d7d194df07de : 是每一个 vCPU 都有 master clock ?
- 9b686a1b-ba34-404b-b639-00ea6b3bed3b : 后面的东西需要搞一下 arm 的
- 683c1fd3-817e-4f6f-a573-a2f85a3a9023 : slab forzen
- 8a86fe89-fa23-4b15-95f8-a56c95cbecba : concurrent 目录整理
- libvirt 的 serial 问题 (不容易哦，先到一个简单的环境中测试吧)
    - 似乎也是可以工作的

## 这也太棒了吧
https://github.com/dwyl/english-words
考虑下如何集成

## 这里添加一个 pr_info，内核无法启动

以前基本经验，任何位置都是可以添加 pr_info 的，算是发现的，第一个
无法添加 pr_info 的地方
```c
static inline void __run_timers(struct timer_base *base)
{
	struct hlist_head heads[LVL_DEPTH];
	int levels;

	if (!time_after_eq(jiffies, base->clk))
		return;

	raw_spin_lock_irq(&base->lock);

	/*
	 * timer_base::must_forward_clk must be cleared before running
	 * timers so that any timer functions that call mod_timer() will
	 * not try to forward the base. Idle tracking / clock forwarding
	 * logic is only used with BASE_STD timers.
	 *
	 * The must_forward_clk flag is cleared unconditionally also for
	 * the deferrable base. The deferrable base is not affected by idle
	 * tracking and never forwarded, so clearing the flag is a NOOP.
	 *
	 * The fact that the deferrable base is never forwarded can cause
	 * large variations in granularity for deferrable timers, but they
	 * can be deferred for long periods due to idle anyway.
	 */
	base->must_forward_clk = false;

	while (time_after_eq(jiffies, base->clk)) {

		levels = collect_expired_timers(base, heads);
		base->clk++;

		while (levels--)
			expire_timers(base, heads + levels);
	}
	base->running_timer = NULL;
	raw_spin_unlock_irq(&base->lock);
}
```

## 都是可以整理的问题

### 构建内核的时候，发现 tune 会使用一个 core 的 80%

### qemu 的 ccls 索引也是会导致 40 ~ 60 的 sys 索引

### 调查一下这个东西

```txt
🤒  sudo cat /dev/ipmi0

~ 🐱
🧀  lsmod | grep -E 'ipmi|serial'
ipmi_ssif              32768  0
ipmi_si                65536  0
ipmi_devintf           20480  0
ipmi_msghandler       110592  3 ipmi_devintf,ipmi_si,ipmi_ssif
```

为什么内核需要这个模块，如何观察其作用?

### 这个内容正好可以整理一下
- plka-chapter-06.md

### ./folio_queue.c 这个实现

### arm 的 idt 问题
```txt
[17586.311727][T13935]  invoke_syscall+0x50/0x120
[17586.316323][T13935]  el0_svc_common.constprop.0+0x48/0xf0
[17586.321867][T13935]  do_el0_svc+0x24/0x38
[17586.326023][T13935]  el0_svc+0x34/0x120
[17586.330003][T13935]  el0t_64_sync_handler+0x10c/0x138
[17586.335191][T13935]  el0t_64_sync+0x190/0x198
```

### 2025 kvm forum 中的东西是没有看完的

### 现在的多线程机制使用 workqueue 是不错的
concurrent/percpu_counter.c
中的代码需要简化一下

### workqueue 中的 sysfs 使用下，并且尝试下 sysfs 中的接口

cpumask 对于多核如何使用来着?


### 这个简单
print_hex_dump_debug


### 看看这个，之前分析 char dev ，感觉非常不好用，各种很杂乱，继续对比看看 block 的使用
unregister_blkdev


### 测试如下函数

void udelay(unsigned long usecs) void ndelay(unsigned long nsecs) void
mdelay(unsigned long msecs) schedule_hrtimeout_range

ktime_get_real_ts64

### 的确可以尝试一下 arm 环境中的 firecracker

### 看看这个放到哪里去吧

```c
// 参考 drivers/tty/sysrq.c:showacpu
static void showacpu(void *dummy)
{
	/* Idle CPUs have no interesting backtrace. */
	// TODO 这个判断真的合理么?
	if (idle_cpu(smp_processor_id()))
		return;
	pr_info("CPU%d:\n", smp_processor_id());
}
```

## pvrdma 有必要看看的
https://mp.weixin.qq.com/s/XTN2m59zNcCWkNdpE17l2Q

## 测试下 init_waitqueue_func_entry

## 有趣的 TODO ，可以先想想再分析
iomap 的实现

### bio 已经描述了 sector 和 page 的关系吗?

## 忽然想到，可以测试一下 vhost scsi 了

既然 targetcli 可以实现


## rsync 可以继续优化下
- 可以利用 inotify 来实现么? 不断的按 w 还是有点傻
- 更多的模板，配合命令 s 使用
- 我希望上传 .git 目录

## nfs 真的对于死锁的这种问题无能为力么?
https://lore.kernel.org/all/87pohqgmh3.fsf@notabene.neil.brown.name/

## 其实可以继续搭建下热迁移的，实现从 67.88 到 hygon 的热迁移

## inoreader 来监控 reddit 或者 hacker news 中的东西

## 部署一下 netdata

## src/c/shm-posix/ 之类的应该记录到 anki 中的

## Documentation/filesystems/sharedsubtree.rst 这个文档不就是用来解决我的痛苦梦魇吗

各种奇怪的 mount 细节

## sosp 的文章都是需要看的
https://mp.weixin.qq.com/s/qF2C0SB1upt-EVmvFbBJ2Q

## iouring 再分析一下
2. 如果 buffer read 不能立刻完成提交，会构建出来新的 thread 来等待吗?

## 这里的东西都可以吸收一下
https://www.moritz.systems/blog/page/4/

## 发现每天 hx9750 开机的时候，13900k 就会有一个这个日志
```txt
[Sun Oct 19 20:22:13 2025] r8169 0000:04:00.0 enp4s0: Link is Up - 1Gbps/Full - flow control off
[Sun Oct 19 20:22:24 2025] r8169 0000:04:00.0 enp4s0: Link is Down
[Sun Oct 19 20:22:27 2025] r8169 0000:04:00.0 enp4s0: Link is Up - 1Gbps/Full - flow control rx/tx
```

## apy 这个项目木看先是不错的

## 13900k 的 kdump 机制搞好吧

```txt
[sudo] password for martins3:
× kdump.service - Crash recovery kernel arming
     Loaded: loaded (/usr/lib/systemd/system/kdump.service; enabled; preset: disabled)
    Drop-In: /usr/lib/systemd/system/service.d
             └─10-timeout-abort.conf
     Active: failed (Result: exit-code) since Sat 2025-10-25 21:12:48 EDT; 11h ago
 Invocation: 17af1cf90f7f42a3b76548da4e4af2c8
    Process: 2072 ExecCondition=/bin/sh -c grep -q -e "crashkernel" -e "fadump" /proc/cmdline (code=exited, status=0>
    Process: 2082 ExecStart=/usr/bin/kdumpctl start (code=exited, status=1/FAILURE)
   Main PID: 2082 (code=exited, status=1/FAILURE)
   Mem peak: 51.3M
        CPU: 2.092s

Oct 25 21:12:48 localhost.localdomain dracut[2458]: Module 'earlykdump' will not be installed, because it's in the l>
Oct 25 21:12:48 localhost.localdomain dracut[2458]: Module 'memstrack' will not be installed, because command 'memst>
Oct 25 21:12:48 localhost.localdomain kdumpctl[2387]: dracut[E]: Module 'squash-squashfs' cannot be installed.
Oct 25 21:12:48 localhost.localdomain dracut[2458]: Module 'squash-squashfs' cannot be installed.
Oct 25 21:12:48 localhost.localdomain kdumpctl[2105]: kdump: mkdumprd: failed to make kdump initrd
Oct 25 21:12:48 localhost.localdomain kdumpctl[2105]: kdump: Starting kdump: [FAILED]
Oct 25 21:12:48 localhost.localdomain systemd[1]: kdump.service: Main process exited, code=exited, status=1/FAILURE
Oct 25 21:12:48 localhost.localdomain systemd[1]: kdump.service: Failed with result 'exit-code'.
Oct 25 21:12:48 localhost.localdomain systemd[1]: Failed to start kdump.service - Crash recovery kernel arming.
Oct 25 21:12:48 localhost.localdomain systemd[1]: kdump.service: Consumed 2.092s CPU time, 51.3M memory peak.
```
似乎只是 squash-squashfs 没有了

## 这个 events 是什么意思?
```txt
1 I root          10       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/0:0H-events_highpri]
1 I root          27       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/2:0H-events_highpri]
1 I root          33       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/4:0H-events_highpri]
1 I root          39       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/6:0H-events_highpri]
1 I root          45       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/8:0H-events_highpri]
1 I root          51       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/10:0H-events_highpri]
1 I root          57       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/12:0H-events_highpri]
1 I root          63       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/14:0H-events_highpri]
1 I root          69       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/16:0H-events_highpri]
1 I root          76       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/17:0H-events_highpri]
1 I root          82       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/18:0H-events_highpri]
1 I root          88       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/19:0H-events_highpri]
1 I root          94       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/20:0H-events_highpri]
1 I root         112       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/23:0H-events_highpri]
1 I root         118       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/24:0H-events_highpri]
1 I root         124       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/25:0H-events_highpri]
1 I root         130       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/26:0H-events_highpri]
1 I root         136       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/27:0H-events_highpri]
1 I root         142       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/28:0H-events_highpri]
1 I root         148       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/29:0H-events_highpri]
1 I root         154       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/30:0H-events_highpri]
1 I root         160       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/31:0H-events_highpri]
1 I root         166       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/1:0H-events_highpri]
1 I root         172       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/3:0H-events_highpri]
1 I root         178       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/5:0H-events_highpri]
1 I root         184       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/7:0H-events_highpri]
1 I root         190       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/9:0H-events_highpri]
1 I root         196       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/11:0H-events_highpri]
1 I root         202       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/13:0H-events_highpri]
1 I root         208       2  0  60 -20 -     0 -      Oct25 ?        00:00:00 [kworker/15:0H-events_highpri]
1 I root      584149       2  0  80   0 -     0 -      16:05 ?        00:00:00 [kworker/26:2-events]
1 I root      593145       2  0  80   0 -     0 -      17:26 ?        00:00:00 [kworker/30:2-events]
1 I root      599651       2  0  80   0 -     0 -      18:39 ?        00:00:00 [kworker/16:0-events]
1 I root      601698       2  0  80   0 -     0 -      19:15 ?        00:00:00 [kworker/28:1-events]
1 I root      602254       2  0  80   0 -     0 -      19:27 ?        00:00:00 [kworker/0:0-events]
1 I root      602974       2  0  80   0 -     0 -      19:30 ?        00:00:00 [kworker/21:1-events]
1 I root      606072       2  0  80   0 -     0 -      19:39 ?        00:00:00 [kworker/29:26-events]
1 I root      608940       2  0  80   0 -     0 -      19:53 ?        00:00:00 [kworker/4:2-events]
1 I root      609055       2  0  80   0 -     0 -      19:55 ?        00:00:00 [kworker/11:1-events_power_efficient]
1 I root      609155       2  0  80   0 -     0 -      19:58 ?        00:00:00 [kworker/12:0-events]
1 I root      610045       2  0  80   0 -     0 -      20:05 ?        00:00:00 [kworker/10:0-events]
1 I root      612768       2  0  80   0 -     0 -      20:11 ?        00:00:00 [kworker/14:2-events]
1 I root      613144       2  0  80   0 -     0 -      20:13 ?        00:00:00 [kworker/10:2-events]
1 I root      614109       2  0  80   0 -     0 -      20:18 ?        00:00:00 [kworker/2:1-events]
1 I root      617302       2  0  80   0 -     0 -      20:46 ?        00:00:00 [kworker/u128:4-events_unbound]
1 I root      618046       2  0  80   0 -     0 -      20:53 ?        00:00:00 [kworker/22:2-events]
1 I root      618596       2  0  80   0 -     0 -      21:02 ?        00:00:00 [kworker/22:3-events]
1 I root      619806       2  0  80   0 -     0 -      21:10 ?        00:00:00 [kworker/u128:7-events_unbound]
1 I root      619807       2  0  80   0 -     0 -      21:10 ?        00:00:00 [kworker/u128:8-events_unbound]
```

## docs/kernel/fault-inject.md 中的东西应该整理下
已经是唾手可得了

## 这两个东西应该是见到很多次了
```txt
# CONFIG_MODULE_ALLOW_BTF_MISMATCH is not set
# CONFIG_BPF_LSM is not set
```

## 把 docs/kernel/blk/mq/plug.md 整理一下吧，里面的东西太多了

## docs/qemu/migration/object.rom.md 也是需要整理一下

### clear_cpuid 这个只是给 xsave 相关的功能使用的
也许不是，但是似乎之前的确就只是在 aux 的禁用中成功过

## 测试一下 vfio 和 vfio-user 的东西吧
qemu 中添加了一个文档 : docs/interop/vfio-user.rst

## IOCTL 接口
```c
#include <linux/fs.h>
#include <linux/ioctl.h>

#define MY_MAGIC 'M'
#define MY_CMD   _IOW(MY_MAGIC, 0, struct my_args)

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case MY_CMD:
        // 处理命令
        break;
    default:
        return -EINVAL;
    }
    return 0;
}
```

## 参考

- Documentation/kbuild/modules.rst
- LKMPG (Linux Kernel Module Programming Guide)
- LDD3 (Linux Device Drivers, 3rd Edition)

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
