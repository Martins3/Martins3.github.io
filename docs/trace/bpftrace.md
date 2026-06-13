# bpftrace

https://github.com/bpftrace/bpftrace

## 先迅速看看例子
```sh
sudo bpftrace -e 'kfunc:vmx_handle_exit { @bytes = hist(args->vcpu->arch.hflags & 1); }'
sudo bpftrace -e 'kfunc:arch_freq_get_on_cpu { printf("-> %d\n", curtask->flags); }'

sudo bpftrace -e 'kretprobe:vfs_read { @bytes = lhist(retval, 0, 2000, 200); }'

# 666 ，这样使用多个 trace 啊
sudo bpftrace -e 'kprobe:vfs_read { @start[tid] = nsecs; } kretprobe:vfs_read /@start[tid]/ { @ns[comm] = hist(nsecs - @start[tid]); delete(@start[tid]); }'

# TODO 如何理解这里的 profile 的功能，对应 bcc 的哪里的东西
sudo bpftrace -e 'profile:hz:99 { @[kstack] = count(); }'
```

## kstack 的三个格式的理解

如果是 perf 格式会显示正好的位置，
```txt
🧀  sudo bpftrace -e "kprobe:do_sys_openat2 { @[kstack(perf)] = count(); }" -c "sleep 10"
Attaching 1 probe...
^C

@[
        ffffffff813ff8f1 do_sys_openat2+1
        ffffffff813fffd5 __x64_sys_openat2+149
        ffffffff821068fc do_syscall_64+188
        ffffffff82200130 entry_SYSCALL_64_after_hwframe+119
]: 30
```

获取到 kallsyms 中:
```txt
ffffffff813ff8f0 t do_sys_openat2
ffffffff82106840 T do_syscall_64
```
可以看到符号右侧的标记就是 该地址相对于符号的地址

## 这里还是需要背下来吧
https://github.com/bpftrace/bpftrace/blob/master/man/adoc/bpftrace.adoc

## 如何可以实时的输出
printf 和 print 都是实时输出的
### [ ] printf 和 print 存在什么区别

## [ ] 可以获取寄存器吗?

## container of 的如何跟踪，看来是需要更加复杂的工具了

```c
static size_t amd_iommu_unmap_pages(struct iommu_domain *dom, unsigned long iova,
				    size_t pgsize, size_t pgcount,
				    struct iommu_iotlb_gather *gather)
{
	struct protection_domain *domain = to_pdomain(dom);
	struct io_pgtable_ops *ops = &domain->iop.iop.ops; //
	size_t r;

	if ((amd_iommu_pgtable == AMD_IOMMU_V1) &&
	    (domain->iop.mode == PAGE_MODE_NONE))
		return 0;

	r = (ops->unmap_pages) ? ops->unmap_pages(ops, iova, pgsize, pgcount, NULL) : 0;

	if (r)
		amd_iommu_iotlb_gather_add_page(dom, gather, iova, r);

	return r;
}
```
https://github.com/iovisor/bpftrace/issues/1455

## 问题 && 代办
### tools/biosnoop.bt 这个工具不可用了

似乎没有什么人用 bpftrace 提供的工具了？
```diff
History:        #0
Commit:         5a80bd075f3bce24793ae1aeb06066895ec5aef0
Author:         Hengqi Chen <hengqi.chen@gmail.com>
Committer:      Jens Axboe <axboe@kernel.dk>
Author Date:    Sat 20 May 2023 04:40:57 PM CST
Committer Date: Wed 24 May 2023 10:38:59 PM CST

block: introduce block_io_start/block_io_done tracepoints

Currently, several BCC ([0]) tools (biosnoop/biostacks/biotop) use
kprobes to blk_account_io_start/blk_account_io_done to implement
their functionalities. This is fragile because the target kernel
functions may be renamed ([1]) or inlined ([2]). So introduce two
new tracepoints for such use cases.

  [0]: https://github.com/iovisor/bcc
  [1]: https://github.com/iovisor/bcc/issues/3954
  [2]: https://github.com/iovisor/bcc/issues/4261

Tested-by: Francis Laniel <flaniel@linux.microsoft.com>
Signed-off-by: Hengqi Chen <hengqi.chen@gmail.com>
Tested-by: Yonghong Song <yhs@fb.com>
Link: https://lore.kernel.org/r/20230520084057.1467003-1-hengqi.chen@gmail.com
Signed-off-by: Jens Axboe <axboe@kernel.dk>
```

### 到底有什么 bpftrace 可以做的?

显然，通过 bcc 都可以做到，但是简化了语法。

https://github.com/bpftrace/bpftrace/blob/master/man/adoc/bpftrace.adoc

### 有趣的两个文档
- https://github.com/bpftrace/bpftrace/blob/master/docs/internals_development.md
- https://github.com/bpftrace/bpftrace/blob/master/docs/fuzzing.md

## 如何遍历数组

```txt
sudo bpftrace -e 'kfunc:vmlinux:call_cpuidle {printf("%s\n", args->drv->states[4].name);}'
```


## 似乎任何全局变量都是可以 trace 的，例如这种
cpuidle_detected_devices 是没有 export 的符号也是可以的
```txt
// https://github.com/brendangregg/bpf-perf-tools-book/blob/master/originals/Ch14_Kernel/loads.bt BEGIN
{
	printf("Reading load averages... Hit Ctrl-C to end.\n");
}

interval:s:1
{
	/*
	 * See fs/proc/loadavg.c and include/linux/sched/loadavg.h for the
	 * following calculations.
	 */
	$avenrun = kaddr("cpuidle_detected_devices");
	$load1 = *$avenrun;
	$load5 = *($avenrun + 8);
	$load15 = *($avenrun + 16);
	time("%H:%M:%S ");
	printf("load averages: %d.%03d %d.%03d %d.%03d\n",
	    ($load1 >> 11), (($load1 & ((1 << 11) - 1)) * 1000) >> 11,
	    ($load5 >> 11), (($load5 & ((1 << 11) - 1)) * 1000) >> 11,
	    ($load15 >> 11), (($load15 & ((1 << 11) - 1)) * 1000) >> 11
	);
}
```

### 如何访问全局变量

之前是不可以的
https://github.com/iovisor/bcc/issues/1207


## [ ]  似乎还有 iter 的封装
```sh
sudo bpftrace -e "iter:tcp { @[kstack] = count(); }"
```
当然，具体如何使用，是一个问题。


## 需求
1. 按照程序名称过滤测试可以吗?
2. 统计一个函数的执行次数，在一段时间中。
  - 不就是 funccount 吗? 但是集成到 bpftrace 中
2. 可以跟踪一个 lock 的使用吗？例如某一个 kvm lock 的使用
3. 如果我同时跟踪所有的 lock 的使用，而且有了 backtrace ，那么岂不是，通过这个方法可以知道了整个 lock 的构型吗?
4. 如果可以动态的跟踪这些 lock ，是否可以做一个 kernel 级别的 dead lock 统计工具


## 为什么 bpftrace 的依赖有 bcc

例如在 src/bpftrace.cpp

```txt
#include bcc/bcc_elf.h>
#include <elf.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/personality.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <bcc/bcc_syms.h>
#include <bcc/perf_reader.h>
```


## bpftrace 也有自己的工具
https://github.com/bpftrace/bpftrace/blob/master/tools/writeback.bt

基本和 bcc 差不多，而且 nixos 没有集成。j

## 想不到 bpftrace 会省去一部分内容
```txt
@[
    ext2_get_blocks+1
    ext2_get_block+94
    do_mpage_readpage+638
    mpage_readahead+158
    ext2_readahead+5 <--- 这个是我手动加上去的
    read_pages+113
    page_cache_ra_unbounded+370
    force_page_cache_ra+147
    filemap_get_pages+314
    filemap_read+251
    vfs_read+665
    ksys_read+110
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 3200
```

以至于让我得出来了一个错误的揭露
原来不是所有的都需要通过 struct file_operations 和 struct address_space_operations 的


## 似乎有时候 bpftrace 中的内容
```txt
@[
    ___slab_alloc+2846
    ___slab_alloc+2846
    __kmalloc_cache_node_noprof+437
    test_deactivate_slab+89
    test_slub+31
    slub_store+101
    kernfs_fop_write_iter+289
    vfs_write+665
    ksys_write+110
    do_syscall_64+95
    entry_SYSCALL_64_after_hwframe+118
]: 126999
```

然后去找对应的地址:
```txt
# 这个找到的是对的
$ l *__kmalloc_cache_node_noprof+437
532
3920             * pointer.
3921             */
3922            c = slub_get_cpu_ptr(s->cpu_slab);
3923    #endif
3924
3925            p = ___slab_alloc(s, gfpflags, node, addr, c, orig_size);
3926    #ifdef CONFIG_PREEMPT_COUNT
3927            slub_put_cpu_ptr(s->cpu_slab);
3928    #endif
3929            return p;

# 但是这个找到的就是错误的:
$ l *___slab_alloc+3101
0xffffffff81542d4d is in ___slab_alloc (mm/slub.c:3845).
3840                        && try_thisnode) {
3841                            try_thisnode = false;
3842                            goto new_objects;
3843                    }
3844                    slab_out_of_memory(s, gfpflags, node);
3845                    return NULL;
3846            }
3847
3848            stat(s, ALLOC_SLAB);
3849
```

## 既然 bpftrace 只能依赖 framepointer
/home/martins3/data/vn/code/src/c/elf/README.md

但是 bpftrace 可以精确的展示内核的 backtrace ?


## 格式化，lsp 高亮都有了
https://github.com/zperf/bpftrace-formatter/

## 有时候为什么是这个错误?

```txt
sudo bpftrace -e 'fentry:vmlinux:bio_submit_split { @[args->split_sectors]=count() }'

Attaching 1 probe...
stdin:1:35-65: WARNING: File exists
Additional Info - helper: map_update_elem, retcode: -17
fentry:vmlinux:bio_submit_split { @[args->split_sectors]=count() }
                                  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
^C

@[256]: 78942
@[-11]: 78943
@[0]: 78946
```

## 这个警告是什么意思?

```txt
 sudo bpftrace -e 'rawtracepoint:x86_fpu_regs_activated { @[curtask->comm] = count() } interval:s:1000 { exit(); }'
[sudo] password for martins3:
Attaching 2 probes...
stdin:1:59-66: WARNING: File exists
Additional Info - helper: map_update_elem, retcode: -17
rawtracepoint:x86_fpu_regs_activated { @[curtask->comm] = count() } interval:s:1000 { exit(); }
                                                          ~~~~~~~
stdin:1:59-66: WARNING: File exists
Additional Info - helper: map_update_elem, retcode: -17
rawtracepoint:x86_fpu_regs_activated { @[curtask->comm] = count() } interval:s:1000 { exit(); }
```

不支持函数指针:
```txt
 sudo bpftrace -v -e 'fentry:vmlinux:__block_write_full_folio { @[args->get_block]=count() }'
Attaching 1 probe...
ERROR: Error loading BPF program for fentry_vmlinux___block_write_full_folio_1.
Kernel error log:
0: R1=ctx() R10=fp0
;  @ bpftrace.bpf.o:0
0: (79) r1 = *(u64 *)(r1 +16)
func '__block_write_full_folio' arg2 type FUNC_PROTO is not a struct
invalid bpf_context access off=16 size=8
processed 1 insns (limit 1000000) max_states_per_insn 0 total_states 0 peak_states 0 mark_read 0


ERROR: Loading BPF object(s) failed.
```

## 测试结果
🧀  sudo bpftrace -lv t:block:block_rq_insert
[sudo] password for martins3:
tracepoint:block:block_rq_insert
    dev_t dev
    sector_t sector
    unsigned int nr_sector
    unsigned int bytes
    unsigned short ioprio
    char rwbs[10]
    char comm[16]
    __data_loc char[] cmd

🧀  sudo bpftrace -lv kfunc:kvm_wait
fentry:vmlinux:kvm_wait
    u8 * ptr
    u8 val

🧀  sudo bpftrace -lv kprobe:kvm_wait
kprobe:kvm_wait
    u8 * arg0
    u8 arg1

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
