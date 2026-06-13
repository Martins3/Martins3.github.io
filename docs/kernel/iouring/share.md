# iouring 内核和用户态如何共享内存
<!-- 25ceff0b-19e2-4e20-81ed-d510559e8d84 -->

TODO 从内核侧如何获取到对应的内存，使用 get user page 吗，就像是 kvm 一样

1. io uring 是如何和内存共享 ring 的
2. aio 共享 ring 的方式也非常奇怪，在内核中创建文件，然后内核中创建 do_mmap

kvm 应该是用的 iommu 来实现换出的，难道 aio 和 iouring 是通过 PIN 实现的

3. 尝试理解下为什么需要
4. 还有这个共享内存: https://mp.weixin.qq.com/s/IOW8UkA1kDw6vanmqxL8KQ

5. ublk 是如何共享内存的

## 分析 aio 如何和用户态共享

在 io_submit 中通过 struct iocb::aio_buf 传递，

将 iocb::aio_buf 指向的用户态数据或者 iovec 传递给内核中 iovec

- aio_setup_rw
  - __import_iovec
    - __import_iovec_ubuf
      - copy_iovec_from_user :

发生在 bio_iov_iter_get_page 类似的位置中，是将内存 pin 住的

- [ ] 应该可以找到对应位置的代码才对

并不是，最后在做 dma 传输的时候，是和 bio_vec 打交道的，而 bio_vec 本身已经包含了
代码:

```c
struct bio_vec {
	struct page	*bv_page;
	unsigned int	bv_len;
	unsigned int	bv_offset;
};
```

## 注意: write / read 也可以 O_DIRECT

```c
static inline int iocb_flags(struct file *file)
{
	int res = 0;
	if (file->f_flags & O_APPEND)
		res |= IOCB_APPEND;
	if (file->f_flags & O_DIRECT)
		res |= IOCB_DIRECT;
	if (file->f_flags & O_DSYNC)
		res |= IOCB_DSYNC;
	if (file->f_flags & __O_SYNC)
		res |= IOCB_SYNC;
	return res;
}
```

__iomap_dio_rw 中判断 IOCB_SYNC 来将 direct io 同时 sync

重新好好看看 open(2)

这里总结的很好:
https://stackoverflow.com/questions/5055859/how-are-the-o-sync-and-o-direct-flags-in-open2-different-alike

1. write / read 使用上 O_DIRECT 如果不带上 O_SYNC 是危险的
2. write / read 也是可以不用拷贝内存的

## [ ] 等等，为什么 aio 为什么需要创建一个 ring 出来了，这个不是共享的吧

```txt
@[
    aio_ring_mmap+5
    mmap_region+584
    do_mmap+992
    ioctx_alloc+970
    __x64_sys_io_setup+55
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 1
```

aio 创建了一个匿名文件 在内核中存放管理 events 的 ring ，这个 ring 并不是共享到用户态的:

```c
static const struct address_space_operations aio_ctx_aops = {
	.dirty_folio	= noop_dirty_folio,
	.migrate_folio	= aio_migrate_folio,
};

static const struct vm_operations_struct aio_ring_vm_ops = {
	.mremap		= aio_ring_mremap,
#if IS_ENABLED(CONFIG_MMU)
	.fault		= filemap_fault,
	.map_pages	= filemap_map_pages,
	.page_mkwrite	= filemap_page_mkwrite,
#endif
};
```

1. 如何使用 aio_context 进行查询 ?

```c
struct aio_ring {
	unsigned	id;	/* kernel internal index number */
	unsigned	nr;	/* number of io_events */
	unsigned	head;	/* Written to by userland or under ring_lock
				 * mutex by aio_read_events_ring(). */
	unsigned	tail;

	unsigned	magic;
	unsigned	compat_features;
	unsigned	incompat_features;
	unsigned	header_length;	/* size of aio_ring */


	struct io_event		io_events[];
}; /* 128 bytes + ring size */
```

1. aio 使用 memory barrier 来同步内存的共享 ?

- aio_read_events_ring 中使用了 smp_rmb ，而 aio_complete 中使用了 smp_wmb 的

- 这里的 Written to by userland 的含义来自于 fa8a53c39f3fdde98c9eace6a9b412143f0f6ed6
  - 当然，看不懂什么叫做 by userland ?
  - 但是，用户态完全不会用到这个结构体。如果会的话，那么 struct aio_ring 就应该定义到 header 中，而不是 aio.c 中
  - 这一套操作只是为了 mremap ，但是如果这个 ring 直接就是内核数据结构，那么的确会遇到一个小问题，就是没有办法 remap
    - 是因为共享到用户态之后，才需要 mremap 吧 !

考虑下，fork 的问题 ?

- 一个 mm_struct 下一组 aioctx

所以，无论如何，分析不清楚为什么存在这种需求。

### iocb 是 aio 特有的数据结构吧

### 所以，为什么 aio 不支持 buffer ？
<!-- 38a83db8-585e-4805-bcfd-63cecd32cc0c -->

感觉这个问题到今天，还是一下子无法想起来

## [ ] 分析下 io uring 的 ring 需要处理 page migration 吗？

务必仔细阅读下: https://man.archlinux.org/man/io_uring.7.en

## io uring 的共享方法

io_uring_mmap 中使用 remap_pfn_range 来配置:

```txt
@[
    io_uring_mmap+5
    mmap_region+584
    do_mmap+992
    vm_mmap_pgoff+239
    ksys_mmap_pgoff+397
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 2
```

- 为什么需要两个队列 ?
  - virtio 似乎就是只是需要一个队列啊
  - 就算是两个队列，其实也是只是需要一次 mmap 吧

### 从 io_tee_prep 来看，到底是和其他的组件如何配合的？

### 实际上 iouring 的 napi 有点问题

在虚拟机中测试，发现了这个

sudo examples/napi-busy-poll-server -l -a 10.0.0.109 -n100000 -p4444 -t10 -b -u

```txt
[  660.543920] kworker/dying (71) used greatest stack depth: 12032 bytes left
[  744.511183] BUG: workqueue lockup - pool cpus=0 node=0 flags=0x0 nice=0 stuck for 51s!
[  744.512420] Showing busy workqueues and worker pools:
[  744.513173] workqueue events: flags=0x0
[  744.513755]   pwq 2: cpus=0 node=0 flags=0x0 nice=0 active=16 refcnt=17
[  744.513756]     pending: vmstat_shepherd, 14*psi_avgs_work, kfree_rcu_monitor
[  744.513764] workqueue events_power_efficient: flags=0x80
[  744.516588]   pwq 2: cpus=0 node=0 flags=0x0 nice=0 active=2 refcnt=3
[  744.516589]     pending: gc_worker, check_lifetime
[  744.516605] workqueue mld: flags=0x40008
[  744.518857]   pwq 2: cpus=0 node=0 flags=0x0 nice=0 active=1 refcnt=2
[  744.518858]     pending: mld_ifc_work
[  744.518862] Showing backtraces of running workers in stalled CPU-bound worker pools:
```

甚至无法正常的 perf 这个程序！

```txt
sudo perf record --call-graph dwarf -p 2583 -- sleep 10
```

## 看看 test/io_uring_register.c 就知道如何共享了

## 分析 io_kiocb 和 aio_kiocb 的构建过程


## mmap 实现的经典例子了
- arch/um/drivers/mmapper_kern.c

## 共享的代价是什么？
带来安全漏洞的可能性
1. vsyscall
2. page table isolation 来解决 intel

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
