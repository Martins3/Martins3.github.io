# RSS
<!-- 83e4aa58-49d1-46c1-ac7f-0b2366294702 -->

总体来说，RSS 机制是一个常用的，如果是本来就理解了 mmap 接口， 容易理解的机制

## 接口

内核中提供了好几个接口，其实一共就三个元素，当然还有很多其他的方法来间接获取，例如
1. oom 报错
2. smaps 中累加各个 vma 区域的

### do_task_stat : /proc/self/stat

```c
static inline unsigned long get_mm_rss(struct mm_struct *mm)
{
    return get_mm_counter(mm, MM_FILEPAGES) +
           get_mm_counter(mm, MM_ANONPAGES) +
           get_mm_counter(mm, MM_SHMEMPAGES);
}
```

### proc_pid_statm -> task_statm : /proc/self/statm

```c
unsigned long task_statm(struct mm_struct *mm,
			 unsigned long *shared, unsigned long *text,
			 unsigned long *data, unsigned long *resident)
{
	*shared = get_mm_counter_sum(mm, MM_FILEPAGES) +
			get_mm_counter_sum(mm, MM_SHMEMPAGES);
	*text = (PAGE_ALIGN(mm->end_code) - (mm->start_code & PAGE_MASK))
								>> PAGE_SHIFT;
	*data = mm->data_vm + mm->stack_vm;
	*resident = *shared + get_mm_counter_sum(mm, MM_ANONPAGES);
	return mm->total_vm;
}
```

```txt
# /proc/PID/statm - 简洁格式（单位：页）
$ cat /proc/self/statm
665 443 88 0 0 0 0
  |   |  |
  |   |  +--- shared (包含 file+shmem)
  |   +------ resident (RSS)
  +---------- size (VSZ)
```

RSS vs VSZ 的区别

 指标                        含义                     包含                                   不包含
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 VSZ (Virtual Memory Size)   虚拟地址空间总大小       所有已映射区域（不论是否分配物理页）   -
 RSS (Resident Set Size)     实际驻留物理内存的页面   已分配物理内存的文件页+匿名页+共享页   已换出页面、未访问的零页

### proc_pid_status -> task_mem : /proc/PID/status

```txt
# /proc/PID/status
$ grep -E 'VmRSS|Rss' /proc/self/status
VmRSS:      1772 kB      # RSS 总量
RssAnon:     108 kB      # 匿名页部分
RssFile:    1652 kB      # 文件映射页部分
RssShmem:     12 kB      # 共享内存页部分
```

## RSS 的三类页面详解

计数器          说明         典型场景
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
MM_ANONPAGES    匿名页       malloc 分配的堆内存、栈、私有匿名映射
MM_FILEPAGES    文件映射页   mmap 映射的文件、可执行文件的代码段
MM_SHMEMPAGES   共享内存页   共享匿名映射(MAP_SHARED|MAP_ANON)、tmpfs

首先回顾一下 mmap-flags.c 中

问题 1 : mmap 到底统计到那里?

配合 rss.c 中的结果，我认为是很清晰了

首先 tmpfs 和 memfd 没有区别，只是可见性的问题:
```txt
╔════════════════════════════════════════════════════════════╗
║                      SUMMARY                               ║
╠════════════════════════════════════════════════════════════╣
║  File Type    │  Map Type   │  After Read  │  After Write ║
╠═══════════════╪═════════════╪══════════════╪══════════════╣
║  Regular File │  PRIVATE    │  RssFile     │  RssAnon     ║
║  Regular File │  SHARED     │  RssFile     │  RssFile     ║
║  tmpfs File   │  PRIVATE    │  RssShmem    │  RssAnon     ║
║  tmpfs File   │  SHARED     │  RssShmem    │  RssShmem    ║
║  Anonymous    │  SHARED     │  N/A         │  RssShmem    ║
╚═══════════════╧═════════════╧══════════════╧══════════════╝
```

我认为其中的关键区别在于，在进行 swap out 的时候，sync to fs 还是需要真的写入到
swap 中去。所以，只有 shared mmap ext4 才算是 MM_FILEPAGES 。
然后分析其他的类型，如果是共享的，那么就是 RssShmem ，如果不可以的，也就是 PRIVATE  map ，然后  cow 的，
那么就统计到 RssAnon 中去

问题 2 :

如果是 open 一个 tmpfs ，然后 write 统计结果是什么样子的?

不会，因为 RSS 统计的是虚拟地址空间的

### 何时增减 RSS

增加 RSS 的场景：

// mm/memory.c - 处理 pf
inc_mm_counter(vma->vm_mm, mm_counter_file(folio));  // 文件页
inc_mm_counter(vma->vm_mm, MM_ANONPAGES);            // 匿名页

减少 RSS 的场景（页面释放/换出时）：

// mm/rmap.c - 页面回收
dec_mm_counter(mm, mm_counter(folio));        // 释放页面

// 页面换出：从 ANONPAGES 移到 SWAPENTS
dec_mm_counter(mm, MM_ANONPAGES);
inc_mm_counter(mm, MM_SWAPENTS);              // 注意：这不计入 RSS

可以看到，这都是 page table 的构建和拆除的时候进行统计的。

## 有趣的问题

memfd + vfio 直通，虚拟机大小为 12G ，结果发现虚拟机的 RSS 是 7193MiB ，一般来说
由于共享机制，一个 process 的 RSS 显示的内存使用比实际要大，但是这是一个例外，因为 vfio 机制自动的 pin 了所有的内存，
但是 qemu 没有去 touch 。

```txt
VM                                                 PID    CAP MiB CURRENT MiB    RSS MiB  AVAIL MiB    RSV MiB  STATE
yyds-fs                                         291205      12288       12288       7193       8294       2457  observe-only: VFIO passthrough
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
