- https://stackoverflow.com/questions/9519648/what-is-the-difference-between-map-shared-and-map-private-in-the-mmap-function
  - MAP_SHARED 和 MAP_PRIVATE 并不是这个 mmap 是否被共享，一定是可以共享的，只是共享之后，其修改是否共享给其他

#### cow
- [ ] 如果可以理解 dirty cow，应该 COW 就没有问题吧 https://dirtycow.ninja/
  - [ ] https://chao-tic.github.io/blog/2017/05/24/dirty-cow : and this one
- [ ] 理解一下文件系统的 cow 的实现 e.g., btrfs
- [ ] check the code related with copying page table when cow
- 1. do_wp_page 和 do_cow_page 是什么关系 ?
    1. do_cow_page : 和 mmap 的第一次创建有关系
2. 什么时候进行 page table 的拷贝 ?

- [ ] do_swap_page is used for read page from anonymous vma, check it's usage

```plain
handle_pte_fault ==>
                    ==> do_wp_page ==> wp_page_copy
do_swap_page     ==>
```

- [ ] do_wp_page
  - [ ] why we should check `PageAnon(vmf->page)` especially
  - [ ] `return VM_FAULT_WRITE;` check why it need return value
  - [ ] `(unlikely((vma->vm_flags & (VM_WRITE|VM_SHARED)) == (VM_WRITE|VM_SHARED))` if a write protection fault can be triggered on the writable page.



- [x] why do_swap_page called do_wp_page ?
  - at first glance, it's unreasonable, but what if a shared page is swapped out and a process it's trying to write to it.
```c
vm_fault_t do_swap_page(struct vm_fault *vmf){
// ...
  if (vmf->flags & FAULT_FLAG_WRITE) {
    ret |= do_wp_page(vmf);
    if (ret & VM_FAULT_ERROR)
      ret &= VM_FAULT_ERROR;
    goto out;
  }
// ...
}
```

- [x] is_cow_mapping : if the page is shared in parent, it's meanless for child to cow it, just access it.
```c
static inline bool is_cow_mapping(vm_flags_t flags)
{
  return (flags & (VM_SHARED | VM_MAYWRITE)) == VM_MAYWRITE;
}
```
- https://stackoverflow.com/questions/48241187/memory-region-flags-in-linux-why-both-vm-write-and-vm-maywrite-are-needed
- https://stackoverflow.com/questions/13405453/shared-memory-pages-and-fork


This is a interesting question, if we want to protect a page being written by cow,
- if the page is writable, so we should clear the writable flags of it ?
- but if the page is not writable, so we should fail the cow page fault ?

[内存在父子进程间的共享时间及范围](https://www.cnblogs.com/tsecer/p/10487840.html)

```plain
sys_fork--->>>>do_fork--->>>copy_process---->>>copy_mm---->>>dup_mm---->>>dup_mmap---->>>copy_page_range---->>>>copy_pud_range---->>>copy_pmd_range---->>>copy_pte_range
---->>>copy_nonpresent_pte
---->>>copy_present_pte
    ---->>> copy_present_page
```
> mmap 时它 mmap 的是私有的，这一点就导致 is_cow_mapping 中 VM_SHARED 是没有置位，因此函数返回值为 true；对于代码段中的空间，它的 VM_SHARED 是满足的，所以函数返回 false，进而导致父进程和子进程直接共享页面，不会设置 COW 属性。


- [x] copy_nonpresent_pte : copy swap entry, migration entry and device entry
- [x] copy_present_pte
- [x] copy_present_page : really simple without dma

# Dirty Cow 漏洞的复现和利用

## 前言
操作系统的安全性是整个系统的安全基础，及时发现并且修复操作系统的安全漏洞因此至关重要，本次试验，我们将复现 `dirtycow <https://dirtycow.ninja/>`_ 漏洞，并且使用内核调试工具分析该漏洞。 Linux 内核开发一方面需要内核开发人员对于系统的原理存在深刻的理解，但是借助各种现代化的分析工具来定位性能瓶颈和潜在的漏洞。

Copy on write 机制是实现高效 fork+exec 组合的必要基础，例如大家常用的 shell，当执行的新的命令的时候，首先会 fork 出来一个新的 shell 进程，然后 exec 需要执行的进程的命令。

```txt
  $ strace ls
  execve("/usr/bin/ls", ["ls"], 0x7ffd44265ab0 /* 81 vars */) = 0
```

Cow 采用的方法是，只是拷贝页表，并且在页表项中间插入 flag，当写对应的页面的时候，会触发 page fault，并且进行拷贝，而读该内存的时候，则需要进行任何操作，直接访问原页面即可。当 fork 出来一个新的页面，如果不采用 cow 机制，大量的页面被拷贝， 这些页面不会被访问，然后由于执行 exec，这些拷贝的页面又立刻被删除，非常的浪费。


内核调试随着内核开发人员的增多而变得愈发的丰富，高效和强大，但是也导致学习相关的工具变得复杂，可以利用 kprobe，向内核中间动态的插入代码，利用 perf 检查各种内核性能瓶颈，利用静态设置的 TracePoint 实现各种数据的收集，甚至可以利用 uprobe 实现动态用户程序的插入。

## Cow

Cow pte 的产生

当进行 fork/clone/vfork 之类的系统调用的时候，其处理内存拷贝的过程大致如下:

1. `_do_fork` : 真正进行 fork 操作的开始，各种系统调用的函数最终在此处汇集。
2. `copy_process` : 进行各种 flags 检查，防止出现互相冲突的 flag，利用 flag 确定到底什么需要进行拷贝，当 flag 中含有 CLONE_VM 的时候，会利用 copy_mm 进行进程地址空间的拷贝。
3. `copy_mm` : 初始化 `mm_struct` 等
4. `dup_mmap` : 逐个 `vm_area` 拷贝，并且构建 `vm_area` 属性
5. `copy_page_range` => `copy_p4d_range` => `copy_pud_range` => copy_pmd_range => copy_pte_range => copy_one_pte : 逐级的进行页表拷贝


注意 ``copy_one_pte`` 其中的一段:

```c
    /*
     * If it's a COW mapping, write protect it both
     * in the parent and the child
     */
    if (is_cow_mapping(vm_flags) && pte_write(pte)) {
        ptep_set_wrprotect(src_mm, addr, src_pte);
        pte = pte_wrprotect(pte);
    }
```

顺便列出其中各种辅助函数的内容:


```c
  static inline bool is_cow_mapping(vm_flags_t flags)
  {
    return (flags & (VM_SHARED | VM_MAYWRITE)) == VM_MAYWRITE;
  }


  static inline int pte_write(pte_t pte)
  {
    return pte_flags(pte) & _PAGE_RW;
  }


  static inline void ptep_set_wrprotect(struct mm_struct *mm,
                unsigned long addr, pte_t *ptep)
  {
    clear_bit(_PAGE_BIT_RW, (unsigned long *)&ptep->pte);
  }

  static inline pte_t pte_wrprotect(pte_t pte)
  {
    return pte_clear_flags(pte, _PAGE_RW);
  }
```

到此，这地清楚了，当拷贝的时候，物理页面并不会进行拷贝，而是仅仅拷贝 pte, 并且将其中写权限去掉，如此，当真正想要进行写的时候，cow page 就需要产生，从而实现原始的 page 不会被破坏。

### Cow page 的产生

当需要写入到到没有一个没有写权限的页面的时候，此时需要产生一个新的物理页面，并且将该物理页面拷贝原来的页面的内容，从而实现修改，其过程为:

在 ``handle_pte_fault`` 中间，当检查到 vmf->flags 中间存在 FAULT_FLAG_WRITE，表示该 page fault 需要进行写操作，但是，pte 权限并不存在写权限，那么就会进行 ``do_wp_page`` 来创建 cow page。


```c

    if (vmf->flags & FAULT_FLAG_WRITE) {plainplainplainplainplainplainplainplain
        if (!pte_write(entry))
            return do_wp_page(vmf);
        entry = pte_mkdirty(entry);
    }
```

### Dirty Cow
下面使用 `这个作为例子 <https://github.com/dirtycow/dirtycow.github.io/blob/master/dirtyc0w.c>` ，其代码比较简单:

1. 接受两个参数，一个没有权限写的目标文件，一个需要利用 `dirty cow` 漏洞用于写入的
2. 主进程将一个以只读的方式将目标文件 mmap 到自己的进程地址空间。
3. 创建两个 thread，第一个循环调用 madvise，叫做 madviseThread，而另一个则是打开 /proc/self/mmap，对于目标文件映射到的虚拟地址空间进行写操作，叫做 procselfmemThread

通过查询 man proc(5)

```txt

   /proc/[pid]/mem
          This file can be used to access the pages of a process's memory through open(2), read(2), and lseek(2).

          Permission to access this file is governed by a ptrace access mode PTRACE_MODE_ATTACH_FSCREDS check; see ptrace(2).plainplainplainplainplainplainplainplain
```

检查其权限:

```txt
  $  ~ l /proc/self/mem
  .rw------- shen shen 0 B Sun Jun  7 07:23:01 2020 mem
```

那么，可以知道，procselfmemThread 进行的操作是，对于目标文件映射位置进行写操作，但是我们知道这样的写操作由于权限不够，显然不可能植入字符串写入到目标文件中间，所以让我们理解一下/proc/self/mem 到底如何实现的。

proc 文件系统是一个虚拟文件系统，对于写操作不是和磁盘打交道。内核的虚拟文件系统提供统一接口来实现文件的读写，/proc/self/mem 实现的接口为:


```c
  static const struct file_operations proc_mem_operations = {
    .llseek     = mem_lseek,
    .read       = mem_read,
    .write      = mem_write,
    .open       = mem_open,
    .release    = mem_release,
  };
```

显然， ``mem_write`` 就是对于 /proc/self/mem 进行写的实际调用对象。

```c
  static ssize_t mem_write(struct file *file, const char __user *buf,
         size_t count, loff_t *ppos)
  {
    return mem_rw(file, (char __user*)buf, count, ppos, 1);
  }
```

在 ``mem_rw`` 中间:

1. 分配一个页面作为缓存
2. 循环执行，知道覆盖到所有的数据:
  1. 使用 ``copy_from_user`` 将用户写入的数据全部拷贝到缓存中间
  2. 调用 ``access_remote_vm``

remote 的含义指的是访问其他进程的地址空间。

```c

  /**
   * access_remote_vm - access another process' address space
   * @mm:       the mm_struct of the target address space
   * @addr: start address to access
   * @buf:  source or destination buffer
   * @len:  number of bytes to transfer
   * @gup_flags:    flags modifying lookup behaviour
   * The caller must hold a reference on @mm.
   * Return: number of bytes copied from source to destination.
   */
  int access_remote_vm(struct mm_struct *mm, unsigned long addr,
      void *buf, int len, unsigned int gup_flags)
  {
    return __access_remote_vm(NULL, mm, addr, buf, len, gup_flags);
  }
```

`access_remote_vm` 只是 `__access_remote_vm` 的简单封装函数，后者的注释说，访问其他进程的地址空间，如果不存在，使用 page fault 获取。

```c
  /*
   * Access another process' address space as given in mm.  If non-NULL, use the
   * given task for page fault accounting.
   */
  int __access_remote_vm(struct task_struct *tsk, struct mm_struct *mm,
      unsigned long addr, void *buf, int len, unsigned int gup_flags)
```

其中的关键步骤在于调用 ``get_user_pages_remote`` ，get user page 机制是 `pin` 用户进程地址空间的页面，其实原因非常的自然，当想要对于其他进程的地址空间进行读写的时候，需要保证其页面不会被释放。

经过几个简单的封装函数，到达一个关键位置:

```c
  static long __get_user_pages(struct task_struct *tsk, struct mm_struct *mm,
      unsigned long start, unsigned long nr_pages,
      unsigned int gup_flags, struct page **pages,
      struct vm_area_struct**vmas, int *locked)
  {
    // ...
    retry:
        /*
         * If we have a pending SIGKILL, don't keep faulting pages and
         * potentially allocating memory.
         */
        if (fatal_signal_pending(current)) {
          ret = -EINTR;
          goto out;
        }
        cond_resched();

        page = follow_page_mask(vma, start, foll_flags, &ctx);plainplainplainplain
        if (!page) {
          ret = faultin_page(tsk, vma, start, &foll_flags,
                 locked);
          switch (ret) {
          case 0:
            goto retry;
          case -EBUSY:
            ret = 0;
            fallthrough;
          case -EFAULT:
          case -ENOMEM:
          case -EHWPOISON:
            goto out;
          case -ENOENT:
            goto next_page;
          }
        // ....
        }
  }
```

``__get_user_pages`` 开始会进行一些边界检查，关键是利用 ``follow_page_mask`` 和 ``faultin_page`` 来模拟一般访存过程，其中 ``follow_page_mask`` 模拟硬件的翻译过程，而软件模拟当页面不存在的时候，进行的 page fault。

``follow_page_mask`` 实现的页面翻译的过程，但是存在多种原因，其成功返回，失败的原因和一般的访问进程地址空间失败的原因类似，该地址没有被映射，该地址对应的物理页面不存在或者权限不够。注意，在 procselfmemThread 中间，需要访问是按照只读的方式映射的内核地址，所以此时 ``follow_page_mask`` 必然会不会成功返回，而是遇到权限而失败返回。

``faultin_page`` 将各种 ``FOLL`` 的标志位转化为 ``FAULT_FLAG`` 类型的标志位，然后调用 ``handle_mm_fault``，经过 pgfault 的标准过程，利用 ``do_wp_page`` 来拷贝原来的页面，同时设置该页面为 dirty 的，这就是 dirty cow page。

在 ``faultin_page`` 返回的位置，就是产生 dirty cow 漏洞的位置，当其判断 cow  已经发生，那么其将 flag 中间的 FOLL_WRITE 清理掉，这样在 ``__get_user_pages`` 下一次 retry 的过程中间，即使 ``maybe_mkwrite`` 没有将 cow page 设置为可写，也会导致无限循环。



```c
    /*
     * The VM_FAULT_WRITE bit tells us that do_wp_page has broken COW when
     * necessary, even if maybe_mkwrite decided not to set pte_write. We
     * can thus safely do subsequent page lookups as if they were reads.
     * But only do so when looping for pte_write is futile: in some cases
     * userspace may also be wanting to write to the gotten user page,
     * which a read fault here might prevent (a readonly page might get
     * reCOWed by userspace write).
     */
    if ((ret & VM_FAULT_WRITE) && !(vma->vm_flags & VM_WRITE))
        *flags &= ~FOLL_WRITE;
```

到这里，一切似乎都非常合理，利用 ``follow_page_mask`` 访问，权限不够，产生 cow page，那么第二次重新 ``follow_page_mask`` 的时候，将会访问到 cow page 上，所以 ``__get_user_pages`` 会开心的处理下一个页面。

但是，问题是，第二次访问的时候，去掉了 ``FOLL_WRITE`` ，那么只是按照只读的方法访问的，如果恰好的将新产生 cow page 去掉，那么访问到是用于映射 root 权限文件的物理页面，这次，不会发生创建一个新的页面，并且进行复制。

似乎，到此时剩下一个问题: 如果去掉 cow page ? 注意，我们还有第二个线程 madviseThread

.. note::
  POSIX_MADV_DONTNEED
     The application expects that it will not access the specified address range in the near future.

通过告知内核该页面不会被访问，让内核主动将页面清理掉。

本部分 `参考 <https://chao-tic.github.io/blog/2017/05/24/dirty-cow>`_

trace
-----
trace 功能从下到上，可以粗略的划分为三种:

1. 来源
2. 导出
3. 呈现

来源
**************
第一个完全在内核态，第三个完全在用户空间，自然，导出数据是用户和内核态的中间者。
数据来源在内核态，划分为四种:

1. kprobe
2. uprobe
3. tracepoint
4. perf

kprobe 可以在内核任何两句汇编语言之间插入想要制定的代码，其实现原理是替换掉想要插入位置的汇编代码，并且填入中断跳转，当内核执行到此处的时候，会跳转到指定的位置，当执行完成代码之后，会中断返回。
由于 kprobe 需要动态修改的汇编代码，kprobe 的具体实现非常依赖于架构，在 kernel/kprobe.c 中间提供了统一的 kprobe 事项的处理。

uprobe 的实现机制类似于 kprobe，但是 uprobe 机制是用于跟踪用户的程序，其处理位置在 kernel/events/uprobes.c

tracepoint 是内核静态定义的信息收集机制，其实现非常简单，例如在 mm/vmscan.c:pageout 中间调用的函数 trace_mm_vmscan_writepage，利用挂载到 /sys/kernel/debug/ 的 ftrace，可以找到其提供的服务的文件:

```c
  /sys/kernel/debug/tracing/events/vmscan/mm_vmscan_writepage
```

其输出格式:

```txt
  [shen-pc mm_vmscan_writepage]# cat format
  name: mm_vmscan_writepage
  ID: 509
  format:
    field:unsigned short common_type;   offset:0;   size:2; signed:0;
    field:unsigned char common_flags;   offset:2;   size:1; signed:0;
    field:unsigned char common_preempt_count;   offset:3;   size:1; signed:0;
    field:int common_pid;   offset:4;   size:4; signed:1;

    field:unsigned long pfn;    offset:8;   size:8; signed:0;
    field:int reclaim_flags;    offset:16;  size:4; signed:1;
```

  print fmt: "page=%p pfn=%lu flags=%s", (((struct page *)vmemmap_base) + (REC->pfn)), REC->pfn, (REC->reclaim_flags) ? __print_flags(REC->reclaim_flags, "|", {0x0001u, "RECLAIM_WB_ANON"}, {0x0002u, "RECLAIM_WB_FILE"}, {0x0010u, "RECLAIM_WB_MIXED"}, {0x0004u, "RECLAIM_WB_SYNC"}, {0x0008u, "RECLAIM_WB_ASYNC"} ) : "RECLAIM_WB_NONE"

和 `trace_mm_vmscan_writepage` 内核中间的定义的代码是具有一致性的，具体的细节可以阅读内核代码。tracepoint 其实包括 USDT(user static defined tracepoint), kernel tracepoint(就是刚才的那种)，以及 lltng-ust.

perf 和前面几种不同，其不在于发现逻辑错误，而在于发现性能瓶颈，perf 的具体实现不仅仅依赖于架构，而且依赖于具体的硬件提供的性能计数器。

导出
****
存在多种导出方式。

eBPF 应该是最强力的工具，BPF(Berkeley package filter)起初的目的是在内核中间对于网络包执行，eBPF 对于做了一些扩展(extend)，如今 eBPF 让内核成为虚拟机，而其中可以运行 BPF 程序，
从理论上讲，eBPF 也许可以成为新的内核模块的形式，但是如今其主要作用是调试。编写 eBPF 的方法并不简单，你可以直接手写 eBPF 汇编，或者利用 llvm 从 C 语言编译，然后从中得到 eBPF 汇编，最后插入到内核中间，这些操作非常的繁琐和易于出错，
`bcc <https://github.com/iovisor/bcc>`_ 就是为了解决这一个问题。如果感觉使用 bcc 编写 c 语言过于麻烦，还有一个更加简单的工具，那就是 bpftrace，借助内核 BTF 特性，甚至 bpftrace 可以非常轻易的查看内核结构体的成员。

`perf_event_open` 是一个系统调用，在很多时候，性能计数器的 perf, 导出数据的 `perf_events`，内核工具 perf，以及一个脚本集合 perf-tools 被混为一谈。下面一一介绍，以后可以在不同语境下可以自动区分。 性能计数器的 perf，上一个小节已经讲过，利用硬件性能计数器以及各种机制，可以分析 cache 假共享，访存次数，以及 CPU cycles 数量，perf_event_open 是一个系统调用，具体内容可以查看 man perf_event_open(2)，
`perf_event_open` 导出的数据就是各种性能计数器产生的。perf-tools 包含了各种常用方便的脚本，主要是利用 ftrace 提供的几口。内核工具 perf 是


ftrace 是一个功能十分强大的导出方法，其形式一个 debugfs，一般被 mount 到 /sys/kernel/debug/tracing 上，上面已经演示过如何利用 ftrace 分析 tracepoint 了，其还可以处理 kprobe，uprobe 的数据，除此之外，ftrace 可以任意函数(前提是该函数不能是 inline 的)调用情况，已经函数调用图，
进一步的分析可以参看内核文档。

各种前端工具，例如 sysdig，SystemTap，lltng 也存在自己的导出方法，这里不再一一赘述。

呈现
****
perf 是内核源码 tools/下的一个文件夹，其一个主要侧重于性能分析，kprobe，uprobe 等各种数据也兼顾的。

trace-cmd 是为了解决更加方便的使用 ftrace 接口，例如自动 mount，开机记录前自动清理 trace 等工作，而 kernelshark 是一个基于 trace-cmd 的图形化工具。

sysdig 的一个特性是对于容器的原生支持，非常适合在云计算业务上，例如 kubunate 或者 docker 等基础设施上进行调试。除此之外，其他功能比较齐全。

SystemTap 是最强大的 trace 工具，其甚至提供自定义的语言，但是 SystemTap 的全部功能的实现需要编译内核的时候 CONFIG_DEBUG_INFO 的选项打开，对于一般的发行版而言，这个功能是没有打开的，不过，在添加 dirty cow 漏洞的时候，可以一并加入。

## 试验步骤
实现的步骤的非常麻烦的一步在于环境的搭建，而核心在于在含有 dirty cow 漏洞的内核上，使用各种工具加以探测分析，为今后分析内核大小基础。

### Step 1 : 环境搭建

搭建环境并不简单，由于缺乏相应的知识，如何编译出来一个可以使用的，同时含有 dirty cow bug 的内核，这里

1. 安装 Manjaro 虚拟机，具体的方法存在很多，下面简单的介绍使用 KVM 基于 KVM 的安装方法:

```sh
qemu-img create -f raw manjaor.img 60G

qemu-system-x86_64 -hda manjaro.img  -boot d -cdrom manjaro-desktop-amd64.iso -m 4G -enable-kvm

qemu-system-x86_64 \
-smp 4  -m 8G -enable-kvm \
-drive file=manjaro.img.bk,index=0,media=disk,format=raw
```

这三个命令，第一个创建磁盘，第二个是利用镜像镜像安装，第三个是运行，此时，我们已经获取了一个 manjaro 环境

2. 获取到 manjaro 提供的制作内核包的源代码
.. code:: sh
  git clone https://gitlab.manjaro.org/packages/core/linux57.git

3. 将 bug.patch 放到 linux57 的目录中间，bug.patch 的内容如下:

```diff
diff --git a/mm/gup.c b/mm/gup.c
index 87a6a59fe667..4cce92706c4a 100644
--- a/mm/gup.c
+++ b/mm/gup.c
@@ -431,7 +431,7 @@ static struct page *follow_page_pte(struct vm_area_struct *vma,
  }
  if ((flags & FOLL_NUMA) && pte_protnone(pte))
    goto no_page;
- if ((flags & FOLL_WRITE) && !can_follow_write_pte(pte, flags)) {
+ if ((flags & FOLL_WRITE) && !pte_write(pte)) {
    pte_unmap_unlock(ptep, ptl);
    return NULL;
  }
@@ -908,7 +908,7 @@ static int faultin_page(struct task_struct *tsk, struct vm_area_struct *vma,
   * reCOWed by userspace write).
   */
  if ((ret & VM_FAULT_WRITE) && !(vma->vm_flags & VM_WRITE))
-     *flags |= FOLL_COW;plainplainplainplainplainplainplain
+     *flags &= ~FOLL_WRITE;plainplainplainplainplainplainplainplainplainplain
  return 0;
 }
```

4. 为 bug.patch 在 PKGBUILD 中间进行其他 patch 类似的调整。

5. 编译安装包，中间进行的时间可能需要数个小时:

```sh
  updpkgsums
  makepkg -s
  sudo pacman -U ./linux57-5.7.0-3-x86_64.pkg.tar.xz ./linux57-headers-5.7.0-3-x86_64.pkg.tar.xz
```

6. 重启系统，选择新编译出来的内核
Step 2 : 复现 bug

运行测试 `代码 <https://github.com/dirtycow/dirtycow.github.io/blob/master/dirtyc0w.c>`
